#pragma once

#include "framework/stack/Stack.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/Transform3D.hpp"

#include <cstdint>
#include <glm/vec3.hpp>
#include <lua.h>
#include <lualib.h>
#include <new>

namespace luax::gd3d {
    inline constexpr char const* kTransformMeta = "luax.gd3d.Transform";
    inline constexpr char const* kTransformTypeName = "Transform";
    inline constexpr char const* kMeshMeta = "luax.gd3d.Mesh";
    inline constexpr char const* kMeshTypeName = "Mesh";

    struct MeshHandle {
        std::uint64_t id = 0;
    };

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

    inline void pushTransform(lua_State* L, render3d::Transform const& transform) {
        auto* storage = static_cast<render3d::Transform*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(render3d::Transform), detail::transformTag())
        );
        new (storage) render3d::Transform(transform);
    }

    inline render3d::Transform* checkTransform(lua_State* L, int idx, [[maybe_unused]] char const* method) {
        return static_cast<render3d::Transform*>(luaL_checkudata(L, idx, kTransformMeta));
    }

    inline MeshHandle* checkMeshHandle(lua_State* L, int idx, [[maybe_unused]] char const* method) {
        return static_cast<MeshHandle*>(luaL_checkudata(L, idx, kMeshMeta));
    }

    inline std::uint64_t requireMeshId(lua_State* L, MeshHandle* handle, char const* method) {
        if (handle == nullptr || handle->id == 0) {
            luaL_error(L, "%s: mesh handle is invalid", method);
        }
        return handle->id;
    }
} // namespace luax::gd3d
