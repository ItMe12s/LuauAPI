#pragma once

#include <Geode/utils/string.hpp>
#include <lua.h>
#include <lualib.h>
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
} // namespace luax
