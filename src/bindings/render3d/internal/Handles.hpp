#pragma once

#include "framework/stack/Stack.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/types/Material.hpp"

#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <new>

namespace luax::gd3d {
    inline constexpr char const* kMeshMeta = "luax.gd3d.Mesh";
    inline constexpr char const* kMeshTypeName = "Mesh";
    inline constexpr char const* kMaterialMeta = "luax.gd3d.Material";
    inline constexpr char const* kMaterialTypeName = "Material";
    inline constexpr char const* kTextureMeta = "luax.gd3d.Texture";
    inline constexpr char const* kTextureTypeName = "Texture";

    struct MeshHandle {
        std::uint64_t id = 0;
    };

    struct TextureHandle {
        std::uint64_t id = 0;
    };

    struct MaterialBox {
        std::shared_ptr<render3d::Material> material;
    };

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

    inline void pushTextureHandle(lua_State* L, std::uint64_t id) {
        auto* handle = static_cast<TextureHandle*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(TextureHandle), detail::textureTag())
        );
        handle->id = id;
    }

    inline TextureHandle* checkTextureHandle(lua_State* L, int idx, [[maybe_unused]] char const* method) {
        return static_cast<TextureHandle*>(luaL_checkudata(L, idx, kTextureMeta));
    }

    inline std::uint64_t requireTextureId(lua_State* L, TextureHandle* handle, char const* method) {
        if (handle == nullptr || handle->id == 0) {
            luaL_error(L, "%s: texture handle is invalid", method);
        }
        return handle->id;
    }
} // namespace luax::gd3d
