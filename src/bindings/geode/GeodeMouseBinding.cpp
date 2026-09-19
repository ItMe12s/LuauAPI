#include "EventHandleBinding.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"

#include <Geode/utils/Keyboard.hpp>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <utility>

namespace {
    using namespace luax;

    constexpr char kMouseListenerMeta[] = "luax.MouseInputListenerHandle";

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

    void readMouseInputData(lua_State* L, int idx, char const* context, geode::MouseInputData& data) {
        idx = lua_absindex(L, idx);
        if (!lua_istable(L, idx)) return;

        if (auto value = optNumberField(L, idx, "button", context)) {
            data.button =
                static_cast<geode::MouseInputData::Button>(static_cast<std::uint8_t>(*value));
        }
        if (auto value = optNumberField(L, idx, "action", context)) {
            data.action =
                static_cast<geode::MouseInputData::Action>(static_cast<std::uint8_t>(*value));
        }
        if (auto value = optNumberField(L, idx, "modifiers", context)) {
            data.modifiers = geode::KeyboardModifier(static_cast<std::uint8_t>(*value));
        }
        if (auto value = optNumberField(L, idx, "timestamp", context)) {
            data.timestamp = *value;
        }
    }

    using MouseBinding = events::EventHandleBinding<
        kMouseListenerMeta, detail::mouseListenerTag(), geode::MouseInputData, &pushMouseInputData,
        &readMouseInputData>;

    bool invokeMousePairEvent(
        std::shared_ptr<LuaCallback> const& cb, char const* context, double first, double second
    ) {
        struct Ctx : CallbackStopFlag {
            double first;
            double second;
        } ctx;

        ctx.first = first;
        ctx.second = second;

        return invoke2ArgCallback(
            cb,
            context,
            ctx,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, c->first);
                lua_pushnumber(L, c->second);
            },
            &ctx
        );
    }

    int mouseInputListen(lua_State* L) {
        return MouseBinding::registerListener(
            L,
            1,
            2,
            "geode.MouseInputEvent.listen",
            [](std::shared_ptr<LuaCallback> const& cb, int priority) {
                return geode::MouseInputEvent().listen(
                    [cb](geode::MouseInputData& data) {
                        return MouseBinding::invoke(cb, "geode.MouseInputEvent.listen", data);
                    },
                    priority
                );
            }
        );
    }

    int mouseMoveListen(lua_State* L) {
        return MouseBinding::registerListener(
            L,
            1,
            2,
            "geode.MouseMoveEvent.listen",
            [](std::shared_ptr<LuaCallback> const& cb, int priority) {
                return geode::MouseMoveEvent().listen(
                    [cb](std::int32_t x, std::int32_t y) {
                        return invokeMousePairEvent(cb, "geode.MouseMoveEvent.listen", x, y);
                    },
                    priority
                );
            }
        );
    }

    int scrollWheelListen(lua_State* L) {
        return MouseBinding::registerListener(
            L,
            1,
            2,
            "geode.ScrollWheelEvent.listen",
            [](std::shared_ptr<LuaCallback> const& cb, int priority) {
                return geode::ScrollWheelEvent().listen(
                    [cb](double xOffset, double yOffset) {
                        return invokeMousePairEvent(
                            cb, "geode.ScrollWheelEvent.listen", xOffset, yOffset
                        );
                    },
                    priority
                );
            }
        );
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
        MouseBinding::registerListenerMetatable(L);
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
