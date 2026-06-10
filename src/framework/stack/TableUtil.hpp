#pragma once

#include "framework/stack/Stack.hpp"

#include <Geode/utils/string.hpp>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace luax {
    inline void getOrCreateTable(lua_State* L, std::string_view path) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        auto segments = geode::utils::string::split(std::string(path), ".");
        for (auto const& seg : segments) {
            lua_pushlstring(L, seg.data(), seg.size());
            lua_rawget(L, -2);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                lua_createtable(L, 0, 4);
                lua_pushlstring(L, seg.data(), seg.size());
                lua_pushvalue(L, -2);
                lua_rawset(L, -4);
            }
            lua_remove(L, -2);
        }
    }

    inline void setTableCFunction(lua_State* L, int tableIdx, char const* name, lua_CFunction fn) {
        lua_pushcfunction(L, fn, name);
        lua_setfield(L, tableIdx < 0 ? tableIdx - 1 : tableIdx, name);
    }

    inline void setIntField(lua_State* L, char const* name, int value) {
        lua_pushinteger(L, value);
        lua_setfield(L, -2, name);
    }

    inline void setLongField(lua_State* L, char const* name, long value) {
        lua_pushnumber(L, static_cast<double>(value));
        lua_setfield(L, -2, name);
    }

    inline bool optField(lua_State* L, int tableIdx, char const* key) {
        lua_getfield(L, tableIdx, key);
        bool present = !lua_isnil(L, -1);
        lua_pop(L, 1);
        return present;
    }

    inline std::optional<std::string> optStringField(
        lua_State* L, int tableIdx, char const* key, char const* method
    ) {
        lua_getfield(L, tableIdx, key);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        auto value = check<std::string>(L, -1, method);
        lua_pop(L, 1);
        return value;
    }

    inline std::optional<bool> optBoolField(
        lua_State* L, int tableIdx, char const* key, char const* method
    ) {
        lua_getfield(L, tableIdx, key);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        auto value = check<bool>(L, -1, method);
        lua_pop(L, 1);
        return value;
    }

    inline std::optional<double> optNumberField(
        lua_State* L, int tableIdx, char const* key, char const* method
    ) {
        lua_getfield(L, tableIdx, key);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        if (!lua_isnumber(L, -1)) luaL_error(L, "%s expected number field '%s'", method, key);
        auto value = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return value;
    }
} // namespace luax
