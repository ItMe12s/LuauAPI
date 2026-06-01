#pragma once

#include "Stack.hpp"

#include <cocos2d.h>
#include <lua.h>

namespace luax {
    template <>
    inline cocos2d::CCPoint check<cocos2d::CCPoint>(lua_State* L, int idx, char const* method) {
        return { fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method) };
    }

    template <>
    inline cocos2d::CCSize check<cocos2d::CCSize>(lua_State* L, int idx, char const* method) {
        return { fieldNumber(L, idx, "width", method), fieldNumber(L, idx, "height", method) };
    }

    template <>
    inline cocos2d::CCRect check<cocos2d::CCRect>(lua_State* L, int idx, char const* method) {
        return {
            { fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method) },
            { fieldNumber(L, idx, "width", method), fieldNumber(L, idx, "height", method) },
        };
    }

    template <>
    inline cocos2d::ccColor3B check<cocos2d::ccColor3B>(lua_State* L, int idx, char const* method) {
        auto r = static_cast<unsigned char>(fieldNumber(L, idx, "r", method));
        auto g = static_cast<unsigned char>(fieldNumber(L, idx, "g", method));
        auto b = static_cast<unsigned char>(fieldNumber(L, idx, "b", method));
        return { r, g, b };
    }

    template <>
    inline UIButtonConfig check<UIButtonConfig>(lua_State* L, int idx, char const* method) {
        UIButtonConfig config{};
        config.m_width = static_cast<int>(fieldNumber(L, idx, "width", method));
        config.m_height = static_cast<int>(fieldNumber(L, idx, "height", method));
        config.m_deadzone = static_cast<float>(fieldNumber(L, idx, "deadzone", method));
        config.m_scale = static_cast<float>(fieldNumber(L, idx, "scale", method));
        config.m_opacity = static_cast<int>(fieldNumber(L, idx, "opacity", method));
        config.m_radius = static_cast<float>(fieldNumber(L, idx, "radius", method));
        config.m_modeB = fieldBool(L, idx, "modeB", method);
        config.m_snap = fieldBool(L, idx, "snap", method);
        lua_getfield(L, idx, "position");
        config.m_position = check<cocos2d::CCPoint>(L, -1, method);
        lua_pop(L, 1);
        config.m_oneButton = fieldBool(L, idx, "oneButton", method);
        config.m_player2 = fieldBool(L, idx, "player2", method);
        config.m_split = fieldBool(L, idx, "split", method);
        return config;
    }

    inline void push(lua_State* L, cocos2d::CCPoint const& point) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, point.x); lua_setfield(L, -2, "x");
        lua_pushnumber(L, point.y); lua_setfield(L, -2, "y");
    }

    inline void push(lua_State* L, cocos2d::CCSize const& size) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, size.width);  lua_setfield(L, -2, "width");
        lua_pushnumber(L, size.height); lua_setfield(L, -2, "height");
    }

    inline void push(lua_State* L, cocos2d::CCRect const& rect) {
        lua_createtable(L, 0, 2);
        push(L, rect.origin); lua_setfield(L, -2, "origin");
        push(L, rect.size); lua_setfield(L, -2, "size");
    }

    inline void push(lua_State* L, cocos2d::ccColor3B const& color) {
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, color.r); lua_setfield(L, -2, "r");
        lua_pushinteger(L, color.g); lua_setfield(L, -2, "g");
        lua_pushinteger(L, color.b); lua_setfield(L, -2, "b");
    }

    inline void push(lua_State* L, UIButtonConfig const& config) {
        lua_createtable(L, 0, 12);
        lua_pushinteger(L, config.m_width); lua_setfield(L, -2, "width");
        lua_pushinteger(L, config.m_height); lua_setfield(L, -2, "height");
        lua_pushnumber(L, config.m_deadzone); lua_setfield(L, -2, "deadzone");
        lua_pushnumber(L, config.m_scale); lua_setfield(L, -2, "scale");
        lua_pushinteger(L, config.m_opacity); lua_setfield(L, -2, "opacity");
        lua_pushnumber(L, config.m_radius); lua_setfield(L, -2, "radius");
        luax::push(L, config.m_modeB); lua_setfield(L, -2, "modeB");
        luax::push(L, config.m_snap); lua_setfield(L, -2, "snap");
        push(L, config.m_position); lua_setfield(L, -2, "position");
        luax::push(L, config.m_oneButton); lua_setfield(L, -2, "oneButton");
        luax::push(L, config.m_player2); lua_setfield(L, -2, "player2");
        luax::push(L, config.m_split); lua_setfield(L, -2, "split");
    }
}
