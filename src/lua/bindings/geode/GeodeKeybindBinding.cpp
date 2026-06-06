#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/framework/Usertype.hpp"

#include <Geode/utils/Keyboard.hpp>
#include <cocos2d.h>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <string>

namespace {
    using namespace luax;

    int intField(lua_State* L, int tableIdx, char const* key, int def, bool required, char const* method) {
        lua_getfield(L, tableIdx, key);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            if (required) luaL_error(L, "%s expected number field '%s'", method, key);
            return def;
        }
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            luaL_error(L, "%s expected number field '%s'", method, key);
        }
        int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    geode::Keybind buildKeybind(lua_State* L, int idx, char const* method) {
        luaL_checktype(L, idx, LUA_TTABLE);
        int key = intField(L, idx, "key", 0, true, method);
        int modifiers = intField(L, idx, "modifiers", 0, false, method);
        return geode::Keybind(
            static_cast<cocos2d::enumKeyCodes>(key),
            geode::KeyboardModifier(static_cast<std::uint8_t>(modifiers))
        );
    }

    void pushKeybind(lua_State* L, geode::Keybind const& keybind) {
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, static_cast<int>(keybind.key));
        lua_setfield(L, -2, "key");
        lua_pushinteger(L, static_cast<int>(keybind.modifiers));
        lua_setfield(L, -2, "modifiers");
    }

    int kbFromString(lua_State* L) {
        auto str = check<std::string>(L, 1, "geode.Keybind.fromString");
        auto result = geode::Keybind::fromString(str);
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
        }
        pushKeybind(L, result.unwrap());
        return 1;
    }

    int kbToString(lua_State* L) {
        auto keybind = buildKeybind(L, 1, "geode.Keybind.toString");
        push(L, keybind.toString());
        return 1;
    }

    int kbCreateNode(lua_State* L) {
        auto keybind = buildKeybind(L, 1, "geode.Keybind.createNode");
        auto* node = keybind.createNode();
        if (!node) {
            lua_pushnil(L);
            return 1;
        }
        Usertype<cocos2d::CCNode>::pushOwned(L, node);
        return 1;
    }
} // namespace

namespace luax {
    geode::Result<void> registerGeodeKeybind(lua_State* L) {
        getOrCreateTable(L, "geode.Keybind");
        setTableCFunction(L, -1, "fromString", &kbFromString);
        setTableCFunction(L, -1, "toString", &kbToString);
        setTableCFunction(L, -1, "createNode", &kbCreateNode);
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_keybind_lib, registerGeodeKeybind)
#endif
