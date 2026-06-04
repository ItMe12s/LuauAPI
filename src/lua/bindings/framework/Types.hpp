#pragma once

#include "Stack.hpp"
#include "Usertype.hpp"

#include <cocos2d.h>
#include <lua.h>

namespace luax {
    template <>
    inline cocos2d::CCPoint check<cocos2d::CCPoint>(lua_State* L, int idx, char const* method) {
        return {fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method)};
    }

    template <>
    inline cocos2d::CCSize check<cocos2d::CCSize>(lua_State* L, int idx, char const* method) {
        return {fieldNumber(L, idx, "width", method), fieldNumber(L, idx, "height", method)};
    }

    template <>
    inline cocos2d::CCRect check<cocos2d::CCRect>(lua_State* L, int idx, char const* method) {
        return {
            {fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method)},
            {fieldNumber(L, idx, "width", method), fieldNumber(L, idx, "height", method)},
        };
    }

    template <>
    inline cocos2d::ccColor3B check<cocos2d::ccColor3B>(lua_State* L, int idx, char const* method) {
        auto r = static_cast<unsigned char>(fieldNumber(L, idx, "r", method));
        auto g = static_cast<unsigned char>(fieldNumber(L, idx, "g", method));
        auto b = static_cast<unsigned char>(fieldNumber(L, idx, "b", method));
        return {r, g, b};
    }

    template <>
    inline cocos2d::ccColor4B check<cocos2d::ccColor4B>(lua_State* L, int idx, char const* method) {
        auto r = static_cast<unsigned char>(fieldNumber(L, idx, "r", method));
        auto g = static_cast<unsigned char>(fieldNumber(L, idx, "g", method));
        auto b = static_cast<unsigned char>(fieldNumber(L, idx, "b", method));
        auto a = static_cast<unsigned char>(fieldNumber(L, idx, "a", method));
        return {r, g, b, a};
    }

    template <>
    inline cocos2d::ccColor4F check<cocos2d::ccColor4F>(lua_State* L, int idx, char const* method) {
        return {
            static_cast<GLfloat>(fieldNumber(L, idx, "r", method)),
            static_cast<GLfloat>(fieldNumber(L, idx, "g", method)),
            static_cast<GLfloat>(fieldNumber(L, idx, "b", method)),
            static_cast<GLfloat>(fieldNumber(L, idx, "a", method)),
        };
    }

    template <>
    inline cocos2d::ccBlendFunc check<cocos2d::ccBlendFunc>(lua_State* L, int idx, char const* method) {
        return {
            static_cast<GLenum>(fieldNumber(L, idx, "src", method)),
            static_cast<GLenum>(fieldNumber(L, idx, "dst", method)),
        };
    }

    template <>
    inline cocos2d::ccHSVValue check<cocos2d::ccHSVValue>(lua_State* L, int idx, char const* method) {
        return {
            static_cast<float>(fieldNumber(L, idx, "h", method)),
            static_cast<float>(fieldNumber(L, idx, "s", method)),
            static_cast<float>(fieldNumber(L, idx, "v", method)),
            fieldBool(L, idx, "absoluteSaturation", method),
            fieldBool(L, idx, "absoluteBrightness", method),
        };
    }

    template <>
    inline cocos2d::CCAffineTransform check<cocos2d::CCAffineTransform>(
        lua_State* L, int idx, char const* method
    ) {
        return {
            static_cast<float>(fieldNumber(L, idx, "a", method)),
            static_cast<float>(fieldNumber(L, idx, "b", method)),
            static_cast<float>(fieldNumber(L, idx, "c", method)),
            static_cast<float>(fieldNumber(L, idx, "d", method)),
            static_cast<float>(fieldNumber(L, idx, "tx", method)),
            static_cast<float>(fieldNumber(L, idx, "ty", method)),
        };
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
        lua_pushnumber(L, point.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, point.y);
        lua_setfield(L, -2, "y");
    }

    inline void push(lua_State* L, cocos2d::CCSize const& size) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, size.width);
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, size.height);
        lua_setfield(L, -2, "height");
    }

    inline void push(lua_State* L, cocos2d::CCRect const& rect) {
        lua_createtable(L, 0, 2);
        push(L, rect.origin);
        lua_setfield(L, -2, "origin");
        push(L, rect.size);
        lua_setfield(L, -2, "size");
    }

    inline void push(lua_State* L, cocos2d::ccColor3B const& color) {
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, color.r);
        lua_setfield(L, -2, "r");
        lua_pushinteger(L, color.g);
        lua_setfield(L, -2, "g");
        lua_pushinteger(L, color.b);
        lua_setfield(L, -2, "b");
    }

    inline void push(lua_State* L, cocos2d::ccColor4B const& color) {
        lua_createtable(L, 0, 4);
        lua_pushinteger(L, color.r);
        lua_setfield(L, -2, "r");
        lua_pushinteger(L, color.g);
        lua_setfield(L, -2, "g");
        lua_pushinteger(L, color.b);
        lua_setfield(L, -2, "b");
        lua_pushinteger(L, color.a);
        lua_setfield(L, -2, "a");
    }

    inline void push(lua_State* L, cocos2d::ccColor4F const& color) {
        lua_createtable(L, 0, 4);
        lua_pushnumber(L, color.r);
        lua_setfield(L, -2, "r");
        lua_pushnumber(L, color.g);
        lua_setfield(L, -2, "g");
        lua_pushnumber(L, color.b);
        lua_setfield(L, -2, "b");
        lua_pushnumber(L, color.a);
        lua_setfield(L, -2, "a");
    }

    inline void push(lua_State* L, cocos2d::ccBlendFunc const& blend) {
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, static_cast<double>(blend.src));
        lua_setfield(L, -2, "src");
        lua_pushnumber(L, static_cast<double>(blend.dst));
        lua_setfield(L, -2, "dst");
    }

    inline void push(lua_State* L, cocos2d::ccHSVValue const& hsv) {
        lua_createtable(L, 0, 5);
        lua_pushnumber(L, hsv.h);
        lua_setfield(L, -2, "h");
        lua_pushnumber(L, hsv.s);
        lua_setfield(L, -2, "s");
        lua_pushnumber(L, hsv.v);
        lua_setfield(L, -2, "v");
        luax::push(L, hsv.absoluteSaturation);
        lua_setfield(L, -2, "absoluteSaturation");
        luax::push(L, hsv.absoluteBrightness);
        lua_setfield(L, -2, "absoluteBrightness");
    }

    inline void push(lua_State* L, cocos2d::CCAffineTransform const& t) {
        lua_createtable(L, 0, 6);
        lua_pushnumber(L, t.a);
        lua_setfield(L, -2, "a");
        lua_pushnumber(L, t.b);
        lua_setfield(L, -2, "b");
        lua_pushnumber(L, t.c);
        lua_setfield(L, -2, "c");
        lua_pushnumber(L, t.d);
        lua_setfield(L, -2, "d");
        lua_pushnumber(L, t.tx);
        lua_setfield(L, -2, "tx");
        lua_pushnumber(L, t.ty);
        lua_setfield(L, -2, "ty");
    }

    inline void push(lua_State* L, UIButtonConfig const& config) {
        lua_createtable(L, 0, 12);
        lua_pushinteger(L, config.m_width);
        lua_setfield(L, -2, "width");
        lua_pushinteger(L, config.m_height);
        lua_setfield(L, -2, "height");
        lua_pushnumber(L, config.m_deadzone);
        lua_setfield(L, -2, "deadzone");
        lua_pushnumber(L, config.m_scale);
        lua_setfield(L, -2, "scale");
        lua_pushinteger(L, config.m_opacity);
        lua_setfield(L, -2, "opacity");
        lua_pushnumber(L, config.m_radius);
        lua_setfield(L, -2, "radius");
        luax::push(L, config.m_modeB);
        lua_setfield(L, -2, "modeB");
        luax::push(L, config.m_snap);
        lua_setfield(L, -2, "snap");
        push(L, config.m_position);
        lua_setfield(L, -2, "position");
        luax::push(L, config.m_oneButton);
        lua_setfield(L, -2, "oneButton");
        luax::push(L, config.m_player2);
        lua_setfield(L, -2, "player2");
        luax::push(L, config.m_split);
        lua_setfield(L, -2, "split");
    }

    template <>
    inline SmartPrefabResult check<SmartPrefabResult>(lua_State* L, int idx, char const* method) {
        SmartPrefabResult result{};
        lua_getfield(L, idx, "smartPrefab");
        if (lua_isnil(L, -1)) {
            result.m_smartPrefab = nullptr;
        }
        else {
            result.m_smartPrefab = Usertype<GJSmartPrefab>::check(L, -1, method);
        }
        lua_pop(L, 1);
        auto binaryKey = fieldString(L, idx, "binaryKey", method);
        result.m_binaryKey = gd::string(binaryKey.c_str());
        auto prefabKey = fieldString(L, idx, "prefabKey", method);
        result.m_prefabKey = gd::string(prefabKey.c_str());
        result.m_prefabCount = static_cast<int>(fieldNumber(L, idx, "prefabCount", method));
        result.m_unrequired = fieldBool(L, idx, "unrequired", method);
        result.m_rotation = static_cast<int>(fieldNumber(L, idx, "rotation", method));
        result.m_flipX = fieldBool(L, idx, "flipX", method);
        result.m_flipY = fieldBool(L, idx, "flipY", method);
        result.m_ignoreCorners = fieldBool(L, idx, "ignoreCorners", method);
        return result;
    }

    inline void push(lua_State* L, SmartPrefabResult const& result) {
        lua_createtable(L, 0, 9);
        if (result.m_smartPrefab == nullptr) {
            lua_pushnil(L);
        }
        else {
            Usertype<GJSmartPrefab>::pushBorrowed(L, result.m_smartPrefab);
        }
        lua_setfield(L, -2, "smartPrefab");
        push(L, std::string(result.m_binaryKey.c_str()));
        lua_setfield(L, -2, "binaryKey");
        push(L, std::string(result.m_prefabKey.c_str()));
        lua_setfield(L, -2, "prefabKey");
        lua_pushinteger(L, result.m_prefabCount);
        lua_setfield(L, -2, "prefabCount");
        luax::push(L, result.m_unrequired);
        lua_setfield(L, -2, "unrequired");
        lua_pushinteger(L, result.m_rotation);
        lua_setfield(L, -2, "rotation");
        luax::push(L, result.m_flipX);
        lua_setfield(L, -2, "flipX");
        luax::push(L, result.m_flipY);
        lua_setfield(L, -2, "flipY");
        luax::push(L, result.m_ignoreCorners);
        lua_setfield(L, -2, "ignoreCorners");
    }
} // namespace luax
