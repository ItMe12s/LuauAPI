#include "EventHandleBinding.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <Geode/loader/Priority.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <utility>

namespace {
    using namespace luax;

    constexpr char kKeyboardListenerMeta[] = "luax.KeyboardInputListenerHandle";

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

    void readKeyboardInputData(lua_State* L, int idx, char const* context, geode::KeyboardInputData& data) {
        idx = lua_absindex(L, idx);
        if (!lua_istable(L, idx)) return;

        if (auto value = optNumberField(L, idx, "key", context)) {
            data.key = static_cast<cocos2d::enumKeyCodes>(static_cast<int>(*value));
        }
        if (auto value = optNumberField(L, idx, "action", context)) {
            data.action =
                static_cast<geode::KeyboardInputData::Action>(static_cast<std::uint8_t>(*value));
        }
        if (auto value = optNumberField(L, idx, "modifiers", context)) {
            data.modifiers = geode::KeyboardModifier(static_cast<std::uint8_t>(*value));
        }
        if (auto value = optNumberField(L, idx, "timestamp", context)) {
            data.timestamp = *value;
        }

        lua_getfield(L, idx, "native");
        if (lua_istable(L, -1)) {
            int nativeIdx = lua_absindex(L, -1);
            if (auto value = optNumberField(L, nativeIdx, "code", context)) {
                data.native.code = static_cast<std::uint64_t>(*value);
            }
            if (auto value = optNumberField(L, nativeIdx, "extra", context)) {
                data.native.extra = static_cast<std::uint64_t>(*value);
            }
        }
        lua_pop(L, 1);
    }

    using KeyboardBinding = events::EventHandleBinding<
        kKeyboardListenerMeta, geode::KeyboardInputData, &pushKeyboardInputData, &readKeyboardInputData>;

    int optPriority(lua_State* L, int idx) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return geode::Priority::Normal;
        return check<int>(L, idx, "geode.KeyboardInputEvent listener");
    }

    int keyboardListen(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 1);
        int priority = optPriority(L, 2);
        auto state = std::make_shared<KeyboardBinding::State>(geode::KeyboardInputEvent().listen(
            [cb](geode::KeyboardInputData& data) {
                return KeyboardBinding::invoke(cb, "geode.KeyboardInputEvent.listen", data);
            },
            priority
        ));
        KeyboardBinding::rememberListener(state);
        KeyboardBinding::pushListener(L, std::move(state));
        return 1;
    }

    int keyboardListenFor(lua_State* L) {
        auto key = check<int>(L, 1, "geode.KeyboardInputEvent.listenFor");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 2);
        int priority = optPriority(L, 3);
        auto state = std::make_shared<KeyboardBinding::State>(
            geode::KeyboardInputEvent(static_cast<cocos2d::enumKeyCodes>(key))
                .listen(
                    [cb](geode::KeyboardInputData& data) {
                        return KeyboardBinding::invoke(cb, "geode.KeyboardInputEvent.listenFor", data);
                    },
                    priority
                )
        );
        KeyboardBinding::rememberListener(state);
        KeyboardBinding::pushListener(L, std::move(state));
        return 1;
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
        KeyboardBinding::registerListenerMetatable(L);
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
