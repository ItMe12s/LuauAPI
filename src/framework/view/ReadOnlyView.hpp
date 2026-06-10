#pragma once

#include <lua.h>
#include <lualib.h>

namespace luax::detail {
    inline void registerReadOnlyMetatable(
        lua_State* L, char const* name, lua_CFunction index, lua_CFunction len,
        lua_CFunction newindex, lua_CFunction gc, char const* typeName
    ) {
        if (luaL_newmetatable(L, name)) {
            lua_pushcfunction(L, index, "__index");
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, len, "__len");
            lua_setfield(L, -2, "__len");
            lua_pushcfunction(L, newindex, "__newindex");
            lua_setfield(L, -2, "__newindex");
            lua_pushcfunction(L, gc, "__gc");
            lua_setfield(L, -2, "__gc");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            lua_pushstring(L, typeName);
            lua_setfield(L, -2, "__type");
        }
        lua_pop(L, 1);
    }
} // namespace luax::detail
