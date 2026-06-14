#pragma once

#include "framework/stack/Stack.hpp"

#include <imgui.h>
#include <lua.h>

namespace luax {
    inline ImVec2 toImVec2(lua_State* L, int idx, char const* method) {
        return ImVec2(fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method));
    }

    inline ImVec4 toImVec4(lua_State* L, int idx, char const* method) {
        return ImVec4(
            fieldNumber(L, idx, "x", method),
            fieldNumber(L, idx, "y", method),
            fieldNumber(L, idx, "z", method),
            fieldNumber(L, idx, "w", method)
        );
    }

    inline void pushImVec2(lua_State* L, ImVec2 const& v) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, v.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, v.y);
        lua_setfield(L, -2, "y");
    }

    inline void pushImVec4(lua_State* L, ImVec4 const& v) {
        lua_createtable(L, 0, 4);
        lua_pushnumber(L, v.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, v.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, v.z);
        lua_setfield(L, -2, "z");
        lua_pushnumber(L, v.w);
        lua_setfield(L, -2, "w");
    }

    inline void pushImVec3(lua_State* L, float x, float y, float z) {
        lua_createtable(L, 0, 3);
        lua_pushnumber(L, x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, z);
        lua_setfield(L, -2, "z");
    }
} // namespace luax
