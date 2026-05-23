#pragma once

#include <lua.h>
#include <lualib.h>

#include <string_view>

namespace luax {
    // Walks a dotted path starting from globals and creates a table for any segment
    // that is missing. The final table is left on top of the stack for the caller.
    inline void getOrCreateTable(lua_State* L, std::string_view path) {
        lua_pushvalue(L, LUA_GLOBALSINDEX);
        size_t start = 0;
        while (start <= path.size()) {
            size_t end = path.find('.', start);
            if (end == std::string_view::npos) end = path.size();
            auto seg = path.substr(start, end - start);
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
            if (end == path.size()) break;
            start = end + 1;
        }
    }

    inline void setTableCFunction(lua_State* L, int tableIdx, char const* name, lua_CFunction fn) {
        lua_pushcfunction(L, fn, name);
        lua_setfield(L, tableIdx < 0 ? tableIdx - 1 : tableIdx, name);
    }
}
