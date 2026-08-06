#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/lifecycle/GeodeListenerState.hpp"
#include "framework/lifecycle/Lifecycle.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"

#include <Geode/loader/Priority.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <new>
#include <optional>
#include <utility>

namespace {
    using namespace luax;

    inline constexpr char const* kMouseListenerMeta = "luax.MouseInputListenerHandle";

    struct MouseListenerState : GeodeListenerState {};

    struct MouseListenerBox {
        std::shared_ptr<MouseListenerState> state;
    };

    WeakHandlePool<MouseListenerState>& activeMouseListeners() {
        static WeakHandlePool<MouseListenerState> listeners;
        return listeners;
    }

    bool& mouseShutdownHookRegistered() {
        static bool registered = false;
        return registered;
    }

    void clearMouseState() {
        activeMouseListeners().clearAll([](MouseListenerState& listener) {
            listener.disconnect();
        });
        mouseShutdownHookRegistered() = false;
    }

    void ensureMouseShutdownHook() {
        ensureShutdownHook(mouseShutdownHookRegistered(), &clearMouseState);
    }

    int optPriority(lua_State* L, int idx) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return geode::Priority::Normal;
        return check<int>(L, idx, "geode mouse event listener");
    }

    bool readNumberField(lua_State* L, int tableIdx, char const* key, double& out) {
        lua_getfield(L, tableIdx, key);
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        out = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return true;
    }

    void pushMouseInputData(lua_State* L, geode::MouseInputData const& data) {
        lua_createtable(L, 0, 4);
        lua_pushinteger(L, static_cast<int>(data.button));
        lua_setfield(L, -2, "button");
        lua_pushinteger(L, static_cast<int>(data.action));
        lua_setfield(L, -2, "action");
        lua_pushinteger(L, static_cast<int>(data.modifiers));
        lua_setfield(L, -2, "modifiers");
        lua_pushnumber(L, data.timestamp);
        lua_setfield(L, -2, "timestamp");
    }

    void readMouseInputData(lua_State* L, int idx, geode::MouseInputData& data) {
        idx = lua_absindex(L, idx);
        if (!lua_istable(L, idx)) return;

        double value = 0.0;
        if (readNumberField(L, idx, "button", value)) {
            data.button =
                static_cast<geode::MouseInputData::Button>(static_cast<std::uint8_t>(value));
        }
        if (readNumberField(L, idx, "action", value)) {
            data.action =
                static_cast<geode::MouseInputData::Action>(static_cast<std::uint8_t>(value));
        }
        if (readNumberField(L, idx, "modifiers", value)) {
            data.modifiers = geode::KeyboardModifier(static_cast<std::uint8_t>(value));
        }
        if (readNumberField(L, idx, "timestamp", value)) {
            data.timestamp = value;
        }
    }

    bool invokeMouseInputEvent(
        std::shared_ptr<LuaCallback> const& cb, char const* context, geode::MouseInputData& data
    ) {
        if (!cb || !cb->valid()) return false;

        struct Ctx {
            geode::MouseInputData* data;
            int dataRef = LUA_NOREF;
            bool stop = false;
        } ctx{&data, LUA_NOREF, false};

        bool ok = cb->invoke(
            1,
            1,
            context,
            kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                pushMouseInputData(L, *c->data);
                lua_pushvalue(L, -1);
                c->dataRef = lua_ref(L, -1);
                lua_pop(L, 1);
            },
            &ctx,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                c->stop = lua_toboolean(L, -1) != 0;
                if (c->dataRef == LUA_NOREF || c->dataRef == LUA_REFNIL) return;
                lua_getref(L, c->dataRef);
                readMouseInputData(L, -1, *c->data);
                lua_pop(L, 1);
            },
            &ctx
        );

        auto* runtime = Runtime::getIfInitialized();
        if (ctx.dataRef != LUA_NOREF && ctx.dataRef != LUA_REFNIL && runtime && runtime->state()) {
            lua_unref(runtime->state(), ctx.dataRef);
        }
        if (!ok) {
            logCallbackFailure(context);
        }
        return ok && ctx.stop;
    }

    bool invokeMousePairEvent(
        std::shared_ptr<LuaCallback> const& cb, char const* context, double first, double second
    ) {
        if (!cb || !cb->valid()) return false;

        struct Ctx {
            double first;
            double second;
            bool stop = false;
        } ctx{first, second, false};

        bool ok = cb->invoke(
            2,
            1,
            context,
            kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, c->first);
                lua_pushnumber(L, c->second);
            },
            &ctx,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                c->stop = lua_toboolean(L, -1) != 0;
            },
            &ctx
        );

        if (!ok) {
            logCallbackFailure(context);
        }
        return ok && ctx.stop;
    }

    void rememberListener(std::shared_ptr<MouseListenerState> const& state) {
        activeMouseListeners().track(state);
        activeMouseListeners().compactAndCountLive();
        ensureMouseShutdownHook();
    }

    void pushListener(lua_State* L, std::shared_ptr<MouseListenerState> state) {
        auto* box = static_cast<MouseListenerBox*>(lua_newuserdata(L, sizeof(MouseListenerBox)));
        new (box) MouseListenerBox{std::move(state)};
        luaL_getmetatable(L, kMouseListenerMeta);
        lua_setmetatable(L, -2);
    }

    MouseListenerBox* checkListener(lua_State* L, int idx) {
        return static_cast<MouseListenerBox*>(luaL_checkudata(L, idx, kMouseListenerMeta));
    }

    int listenerGc(lua_State* L) {
        auto* box = checkListener(L, 1);
        box->~MouseListenerBox();
        return 0;
    }

    int listenerDisconnect(lua_State* L) {
        auto* box = checkListener(L, 1);
        if (box->state) box->state->disconnect();
        return 0;
    }

    int mouseInputListen(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 1);
        auto state = std::make_shared<MouseListenerState>();
        int priority = optPriority(L, 2);
        state->handle = geode::MouseInputEvent().listen(
            [cb](geode::MouseInputData& data) {
                return invokeMouseInputEvent(cb, "geode.MouseInputEvent.listen", data);
            },
            priority
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    int mouseMoveListen(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 1);
        auto state = std::make_shared<MouseListenerState>();
        int priority = optPriority(L, 2);
        state->handle = geode::MouseMoveEvent().listen(
            [cb](std::int32_t x, std::int32_t y) {
                return invokeMousePairEvent(cb, "geode.MouseMoveEvent.listen", x, y);
            },
            priority
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    int scrollWheelListen(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 1);
        auto state = std::make_shared<MouseListenerState>();
        int priority = optPriority(L, 2);
        state->handle = geode::ScrollWheelEvent().listen(
            [cb](double xOffset, double yOffset) {
                return invokeMousePairEvent(cb, "geode.ScrollWheelEvent.listen", xOffset, yOffset);
            },
            priority
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    void registerListenerMetatable(lua_State* L) {
        luaL_Reg methods[] = {
            {"disconnect", listenerDisconnect},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(L, kMouseListenerMeta, std::nullopt, methods, &listenerGc);
    }

    geode::Result<void> registerMouseInputData(lua_State* L) {
        getOrCreateTable(L, "geode.MouseInputData");

        lua_createtable(L, 0, 2);
        setIntField(L, "Press", static_cast<int>(geode::MouseInputData::Action::Press));
        setIntField(L, "Release", static_cast<int>(geode::MouseInputData::Action::Release));
        lua_setfield(L, -2, "Action");

        lua_createtable(L, 0, 5);
        setIntField(L, "Left", static_cast<int>(geode::MouseInputData::Button::Left));
        setIntField(L, "Right", static_cast<int>(geode::MouseInputData::Button::Right));
        setIntField(L, "Middle", static_cast<int>(geode::MouseInputData::Button::Middle));
        setIntField(L, "Button4", static_cast<int>(geode::MouseInputData::Button::Button4));
        setIntField(L, "Button5", static_cast<int>(geode::MouseInputData::Button::Button5));
        lua_setfield(L, -2, "Button");

        lua_pop(L, 1);
        return geode::Ok();
    }

    geode::Result<void> registerMouseInputEvent(lua_State* L) {
        getOrCreateTable(L, "geode.MouseInputEvent");
        setTableCFunction(L, -1, "listen", &mouseInputListen);
        lua_pop(L, 1);
        return geode::Ok();
    }

    geode::Result<void> registerMouseMoveEvent(lua_State* L) {
        getOrCreateTable(L, "geode.MouseMoveEvent");
        setTableCFunction(L, -1, "listen", &mouseMoveListen);
        lua_pop(L, 1);
        return geode::Ok();
    }

    geode::Result<void> registerScrollWheelEvent(lua_State* L) {
        getOrCreateTable(L, "geode.ScrollWheelEvent");
        setTableCFunction(L, -1, "listen", &scrollWheelListen);
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace

namespace luax {
    geode::Result<void> registerGeodeMouseInput(lua_State* L) {
        registerListenerMetatable(L);
        if (auto result = registerMouseInputData(L); result.isErr()) {
            return result;
        }
        if (auto result = registerMouseInputEvent(L); result.isErr()) {
            return result;
        }
        if (auto result = registerMouseMoveEvent(L); result.isErr()) {
            return result;
        }
        if (auto result = registerScrollWheelEvent(L); result.isErr()) {
            return result;
        }
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_mouse_input_lib, registerGeodeMouseInput)
#endif
