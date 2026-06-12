#pragma once

#include "framework/stack/Stack.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/Material.hpp"
#include "render3d/Transform3D.hpp"

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <new>

namespace luax::gd3d {
    inline constexpr char const* kTransformMeta = "luax.gd3d.Transform";
    inline constexpr char const* kTransformTypeName = "Transform";
    inline constexpr char const* kMeshMeta = "luax.gd3d.Mesh";
    inline constexpr char const* kMeshTypeName = "Mesh";
    inline constexpr char const* kMaterialMeta = "luax.gd3d.Material";
    inline constexpr char const* kMaterialTypeName = "Material";

    struct MeshHandle {
        std::uint64_t id = 0;
    };

    struct MaterialBox {
        std::shared_ptr<render3d::Material> material;
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

    inline void pushMaterial(lua_State* L, std::shared_ptr<render3d::Material> material) {
        auto* box = static_cast<MaterialBox*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(MaterialBox), detail::materialTag())
        );
        new (box) MaterialBox{std::move(material)};
    }

    inline MaterialBox* checkMaterialBox(lua_State* L, int idx, [[maybe_unused]] char const* method) {
        return static_cast<MaterialBox*>(luaL_checkudata(L, idx, kMaterialMeta));
    }

    inline std::shared_ptr<render3d::Material> const& requireMaterial(
        lua_State* L, int idx, char const* method
    ) {
        auto* box = checkMaterialBox(L, idx, method);
        if (!box->material) {
            luaL_error(L, "%s: material handle is invalid", method);
        }
        return box->material;
    }
} // namespace luax::gd3d
