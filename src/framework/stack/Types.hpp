#pragma once

#include "framework/stack/Stack.hpp"
#include "framework/stack/Types.generated.hpp"
#include "framework/usertype/Usertype.hpp"

#include <cocos2d.h>
#include <lua.h>

namespace luax {
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
