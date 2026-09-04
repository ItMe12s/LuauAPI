#pragma once

#include "framework/stack/Stack.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/types/Transform3D.hpp"

#include <Geode/Result.hpp>
#include <cstddef>
#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <lua.h>
#include <lualib.h>
#include <new>
#include <span>

namespace luax::gd3d {
    inline constexpr char const* kTransformMeta = "luax.gd3d.Transform";
    inline constexpr char const* kTransformTypeName = "Transform";

    inline glm::vec3 checkVec3(lua_State* L, int idx, char const* method) {
        luaL_checktype(L, idx, LUA_TTABLE);
        return glm::vec3(
            fieldNumber(L, idx, "x", method),
            fieldNumber(L, idx, "y", method),
            fieldNumber(L, idx, "z", method)
        );
    }

    inline void pushVec3(lua_State* L, glm::vec3 const& v) {
        lua_createtable(L, 0, 3);
        lua_pushnumber(L, v.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, v.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, v.z);
        lua_setfield(L, -2, "z");
    }

    inline void pushColor(lua_State* L, glm::vec4 const& color) {
        lua_createtable(L, 0, 4);
        luax::push(L, color.r);
        lua_setfield(L, -2, "r");
        luax::push(L, color.g);
        lua_setfield(L, -2, "g");
        luax::push(L, color.b);
        lua_setfield(L, -2, "b");
        luax::push(L, color.a);
        lua_setfield(L, -2, "a");
    }

    inline glm::vec4 parseColor(lua_State* L, int idx, char const* method) {
        luaL_checktype(L, idx, LUA_TTABLE);
        lua_getfield(L, idx, "r");
        if (!lua_isnil(L, -1)) {
            float const r = static_cast<float>(luaL_checknumber(L, -1));
            lua_pop(L, 1);
            float const g = fieldNumber(L, idx, "g", method);
            float const b = fieldNumber(L, idx, "b", method);
            lua_getfield(L, idx, "a");
            float const a = lua_isnil(L, -1) ? 1.0f : static_cast<float>(luaL_checknumber(L, -1));
            lua_pop(L, 1);
            return glm::vec4(r, g, b, a);
        }
        lua_pop(L, 1);
        return glm::vec4(
            fieldNumber(L, idx, "x", method),
            fieldNumber(L, idx, "y", method),
            fieldNumber(L, idx, "z", method),
            1.0f
        );
    }

    inline geode::Result<std::span<std::uint8_t const>> checkBufferOrString(
        lua_State* L, int idx, char const* method, std::size_t maxBytes, char const* emptyErr,
        char const* sizeErr
    ) {
        std::span<std::uint8_t const> bytes{};
        if (lua_isbuffer(L, idx)) {
            size_t len = 0;
            void* data = lua_tobuffer(L, idx, &len);
            if (data == nullptr || len == 0) {
                return geode::Err(emptyErr);
            }
            bytes = std::span<std::uint8_t const>(static_cast<std::uint8_t const*>(data), len);
        }
        else {
            size_t len = 0;
            char const* text = lua_tolstring(L, idx, &len);
            if (text == nullptr) {
                luaL_error(L, "%s expected buffer or string at arg %d", method, idx);
            }
            if (len == 0) {
                return geode::Err(emptyErr);
            }
            bytes = std::span<std::uint8_t const>(reinterpret_cast<std::uint8_t const*>(text), len);
        }
        if (bytes.size() > maxBytes) {
            return geode::Err(sizeErr);
        }
        return geode::Ok(bytes);
    }

    inline void pushTransform(lua_State* L, render3d::Transform const& transform) {
        auto* storage = static_cast<render3d::Transform*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(render3d::Transform), detail::transformTag())
        );
        new (storage) render3d::Transform(transform);
    }

    inline render3d::Transform* checkTransform(lua_State* L, int idx, [[maybe_unused]] char const* method) {
        return static_cast<render3d::Transform*>(luaL_checkudata(L, idx, kTransformMeta));
    }
} // namespace luax::gd3d
