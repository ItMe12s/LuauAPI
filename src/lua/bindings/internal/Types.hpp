#pragma once

#include "Stack.hpp"

#include <cocos2d.h>
#include <lua.h>

namespace luax {
    inline cocos2d::CCPoint toPoint(lua_State* L, int idx, char const* method) {
        return { fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method) };
    }

    inline cocos2d::CCSize toSize(lua_State* L, int idx, char const* method) {
        return { fieldNumber(L, idx, "width", method), fieldNumber(L, idx, "height", method) };
    }

    inline cocos2d::CCRect toRect(lua_State* L, int idx, char const* method) {
        return {
            { fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method) },
            { fieldNumber(L, idx, "width", method), fieldNumber(L, idx, "height", method) },
        };
    }

    inline cocos2d::ccColor3B toColor3B(lua_State* L, int idx, char const* method) {
        auto r = static_cast<unsigned char>(fieldNumber(L, idx, "r", method));
        auto g = static_cast<unsigned char>(fieldNumber(L, idx, "g", method));
        auto b = static_cast<unsigned char>(fieldNumber(L, idx, "b", method));
        return { r, g, b };
    }

    inline void pushPoint(lua_State* L, cocos2d::CCPoint const& point) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, point.x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, point.y); lua_setfield(L, -2, "y");
    }

    inline void pushSize(lua_State* L, cocos2d::CCSize const& size) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, size.width);  lua_setfield(L, -2, "width");
        lua_pushnumber(L, size.height); lua_setfield(L, -2, "height");
    }

    inline void pushColor3B(lua_State* L, cocos2d::ccColor3B const& color) {
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, color.r); lua_setfield(L, -2, "r");
        lua_pushinteger(L, color.g); lua_setfield(L, -2, "g");
        lua_pushinteger(L, color.b); lua_setfield(L, -2, "b");
    }
}
