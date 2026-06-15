#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/lifecycle/ShutdownHook.hpp"
#include "framework/lifecycle/WeakHandlePool.hpp"
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

    inline constexpr char const* kKeyboardListenerMeta = "luax.KeyboardInputListenerHandle";

    struct KeyboardListenerState {
        geode::ListenerHandle handle;
        bool connected = true;

        ~KeyboardListenerState() {
            disconnect();
        }

        void disconnect() {
            if (!connected) return;
            handle = geode::ListenerHandle();
            connected = false;
        }
    };

    struct KeyboardListenerBox {
        std::shared_ptr<KeyboardListenerState> state;
    };

    WeakHandlePool<KeyboardListenerState>& activeKeyboardListeners() {
        static WeakHandlePool<KeyboardListenerState> listeners;
        return listeners;
    }

    bool& keyboardShutdownHookRegistered() {
        static bool registered = false;
        return registered;
    }

    void clearKeyboardState() {
        activeKeyboardListeners().clearAll([](KeyboardListenerState& listener) {
            listener.disconnect();
        });
        keyboardShutdownHookRegistered() = false;
    }

    void ensureKeyboardShutdownHook() {
        ensureShutdownHook(keyboardShutdownHookRegistered(), &clearKeyboardState);
    }

    int optPriority(lua_State* L, int idx) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return geode::Priority::Normal;
        return check<int>(L, idx, "geode.KeyboardInputEvent listener");
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

    void pushKeyboardInputData(lua_State* L, geode::KeyboardInputData const& data) {
        lua_createtable(L, 0, 5);
        lua_pushinteger(L, static_cast<int>(data.key));
        lua_setfield(L, -2, "key");
        lua_pushinteger(L, static_cast<int>(data.action));
        lua_setfield(L, -2, "action");
        lua_pushinteger(L, static_cast<int>(data.modifiers));
        lua_setfield(L, -2, "modifiers");
        lua_pushnumber(L, data.timestamp);
        lua_setfield(L, -2, "timestamp");

        lua_createtable(L, 0, 2);
        lua_pushnumber(L, static_cast<double>(data.native.code));
        lua_setfield(L, -2, "code");
        lua_pushnumber(L, static_cast<double>(data.native.extra));
        lua_setfield(L, -2, "extra");
        lua_setfield(L, -2, "native");
    }

    void readKeyboardInputData(lua_State* L, int idx, geode::KeyboardInputData& data) {
        idx = lua_absindex(L, idx);
        if (!lua_istable(L, idx)) return;

        double value = 0.0;
        if (readNumberField(L, idx, "key", value)) {
            data.key = static_cast<cocos2d::enumKeyCodes>(static_cast<int>(value));
        }
        if (readNumberField(L, idx, "action", value)) {
            data.action =
                static_cast<geode::KeyboardInputData::Action>(static_cast<std::uint8_t>(value));
        }
        if (readNumberField(L, idx, "modifiers", value)) {
            data.modifiers = geode::KeyboardModifier(static_cast<std::uint8_t>(value));
        }
        if (readNumberField(L, idx, "timestamp", value)) {
            data.timestamp = value;
        }

        lua_getfield(L, idx, "native");
        if (lua_istable(L, -1)) {
            int nativeIdx = lua_absindex(L, -1);
            if (readNumberField(L, nativeIdx, "code", value)) {
                data.native.code = static_cast<std::uint64_t>(value);
            }
            if (readNumberField(L, nativeIdx, "extra", value)) {
                data.native.extra = static_cast<std::uint64_t>(value);
            }
        }
        lua_pop(L, 1);
    }

    bool invokeKeyboardEvent(
        std::shared_ptr<LuaCallback> const& cb, char const* context, geode::KeyboardInputData& data
    ) {
        if (!cb || !cb->valid()) return false;

        struct Ctx {
            geode::KeyboardInputData* data;
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
                pushKeyboardInputData(L, *c->data);
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
                readKeyboardInputData(L, -1, *c->data);
                lua_pop(L, 1);
            },
            &ctx
        );

        auto* host = BindingHost::getIfInitialized();
        if (ctx.dataRef != LUA_NOREF && ctx.dataRef != LUA_REFNIL && host && host->state()) {
            lua_unref(host->state(), ctx.dataRef);
        }
        if (!ok) {
            logCallbackFailure(context);
        }
        return ok && ctx.stop;
    }

    void rememberListener(std::shared_ptr<KeyboardListenerState> const& state) {
        activeKeyboardListeners().track(state);
        activeKeyboardListeners().compactAndCountLive();
        ensureKeyboardShutdownHook();
    }

    void pushListener(lua_State* L, std::shared_ptr<KeyboardListenerState> state) {
        auto* box =
            static_cast<KeyboardListenerBox*>(lua_newuserdata(L, sizeof(KeyboardListenerBox)));
        new (box) KeyboardListenerBox{std::move(state)};
        luaL_getmetatable(L, kKeyboardListenerMeta);
        lua_setmetatable(L, -2);
    }

    KeyboardListenerBox* checkListener(lua_State* L, int idx, char const* method) {
        return static_cast<KeyboardListenerBox*>(luaL_checkudata(L, idx, method));
    }

    int listenerGc(lua_State* L) {
        auto* box = static_cast<KeyboardListenerBox*>(luaL_checkudata(L, 1, kKeyboardListenerMeta));
        box->~KeyboardListenerBox();
        return 0;
    }

    int listenerDisconnect(lua_State* L) {
        auto* box = checkListener(L, 1, kKeyboardListenerMeta);
        if (box->state) box->state->disconnect();
        return 0;
    }

    int keyboardListen(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 1);
        auto state = std::make_shared<KeyboardListenerState>();
        int priority = optPriority(L, 2);
        state->handle = geode::KeyboardInputEvent().listen(
            [cb](geode::KeyboardInputData& data) {
                return invokeKeyboardEvent(cb, "geode.KeyboardInputEvent.listen", data);
            },
            priority
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    int keyboardListenFor(lua_State* L) {
        auto key = check<int>(L, 1, "geode.KeyboardInputEvent.listenFor");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 2);
        auto state = std::make_shared<KeyboardListenerState>();
        int priority = optPriority(L, 3);
        state->handle =
            geode::KeyboardInputEvent(static_cast<cocos2d::enumKeyCodes>(key))
                .listen(
                    [cb](geode::KeyboardInputData& data) {
                        return invokeKeyboardEvent(cb, "geode.KeyboardInputEvent.listenFor", data);
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
        registerTaggedMetatable(L, kKeyboardListenerMeta, std::nullopt, methods, &listenerGc);
    }

    geode::Result<void> registerKeyboardModifier(lua_State* L) {
        getOrCreateTable(L, "geode.KeyboardModifier");
        lua_pushinteger(L, geode::KeyboardModifier::None);
        lua_setfield(L, -2, "None");
        lua_pushinteger(L, geode::KeyboardModifier::Shift);
        lua_setfield(L, -2, "Shift");
        lua_pushinteger(L, geode::KeyboardModifier::Control);
        lua_setfield(L, -2, "Control");
        lua_pushinteger(L, geode::KeyboardModifier::Alt);
        lua_setfield(L, -2, "Alt");
        lua_pushinteger(L, geode::KeyboardModifier::Super);
        lua_setfield(L, -2, "Super");
        lua_pop(L, 1);
        return geode::Ok();
    }

    geode::Result<void> registerKeyboardInputData(lua_State* L) {
        getOrCreateTable(L, "geode.KeyboardInputData");
        lua_createtable(L, 0, 3);
        setIntField(L, "Press", static_cast<int>(geode::KeyboardInputData::Action::Press));
        setIntField(L, "Release", static_cast<int>(geode::KeyboardInputData::Action::Release));
        setIntField(L, "Repeat", static_cast<int>(geode::KeyboardInputData::Action::Repeat));
        lua_setfield(L, -2, "Action");
        lua_pop(L, 1);
        return geode::Ok();
    }

    geode::Result<void> registerKeyboardInputEvent(lua_State* L) {
        getOrCreateTable(L, "geode.KeyboardInputEvent");
        setTableCFunction(L, -1, "listen", &keyboardListen);
        setTableCFunction(L, -1, "listenFor", &keyboardListenFor);
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace

namespace luax {
    geode::Result<void> registerGeodeKeyboardInput(lua_State* L) {
        registerListenerMetatable(L);
        if (auto result = registerKeyboardModifier(L); result.isErr()) {
            return result;
        }
        if (auto result = registerKeyboardInputData(L); result.isErr()) {
            return result;
        }
        if (auto result = registerKeyboardInputEvent(L); result.isErr()) {
            return result;
        }
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_keyboard_input_lib, registerGeodeKeyboardInput)
#endif
