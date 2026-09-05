#pragma once

#include "framework/stack/Stack.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/assets/MeshAsset.hpp"
#include "render3d/assets/TextureAsset.hpp"
#include "render3d/types/Material.hpp"

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

    struct MeshBox {
        std::shared_ptr<render3d::MeshAsset> mesh;
    };

    struct TextureBox {
        std::shared_ptr<render3d::TextureAsset> texture;
    };

    struct MaterialBox {
        std::shared_ptr<render3d::Material> material;
    };

    inline MeshBox* checkMeshHandle(lua_State* L, int idx, char const*) {
        return static_cast<MeshBox*>(luaL_checkudata(L, idx, kMeshMeta));
    }

    inline std::shared_ptr<render3d::MeshAsset> const& requireMesh(
        lua_State* L, MeshBox* box, char const* method
    ) {
        if (!box->mesh) {
            luaL_error(L, "%s: mesh handle is invalid", method);
        }
        return box->mesh;
    }

    inline void pushMeshHandle(lua_State* L, std::shared_ptr<render3d::MeshAsset> mesh) {
        auto* box = static_cast<MeshBox*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(MeshBox), detail::meshAssetTag())
        );
        new (box) MeshBox{std::move(mesh)};
    }

    inline void pushMaterial(lua_State* L, std::shared_ptr<render3d::Material> material) {
        auto* box = static_cast<MaterialBox*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(MaterialBox), detail::materialTag())
        );
        new (box) MaterialBox{std::move(material)};
    }

    inline MaterialBox* checkMaterialBox(lua_State* L, int idx, char const*) {
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

    inline void pushTextureHandle(lua_State* L, std::shared_ptr<render3d::TextureAsset> texture) {
        auto* box = static_cast<TextureBox*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(TextureBox), detail::textureTag())
        );
        new (box) TextureBox{std::move(texture)};
    }

    inline TextureBox* checkTextureHandle(lua_State* L, int idx, char const*) {
        return static_cast<TextureBox*>(luaL_checkudata(L, idx, kTextureMeta));
    }

    inline std::shared_ptr<render3d::TextureAsset> const& requireTexture(
        lua_State* L, TextureBox* box, char const* method
    ) {
        if (!box->texture) {
            luaL_error(L, "%s: texture handle is invalid", method);
        }
        return box->texture;
    }
} // namespace luax::gd3d
