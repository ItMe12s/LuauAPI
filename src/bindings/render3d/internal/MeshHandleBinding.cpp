#include "bindings/render3d/internal/MeshHandleBinding.hpp"

#include "bindings/render3d/internal/Marshaling.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/assets/MeshAsset.hpp"
#include "render3d/types/Material.hpp"
#if !defined(LUAUAPI_HOST_TESTS)
    #include "render3d/gpu/Renderer3D.hpp"
#endif

#include <lua.h>
#include <lualib.h>
#include <memory>

namespace {
    using namespace luax;
    using namespace luax::gd3d;
    using namespace luax::render3d;

    void releaseMeshBox(MeshBox* box) {
#if !defined(LUAUAPI_HOST_TESTS)
        if (box != nullptr && box->mesh) {
            Renderer3D::instance().releaseMeshGpu(box->mesh.get());
        }
#endif
        if (box != nullptr) {
            box->mesh.reset();
        }
    }

    int meshCountAxis(lua_State* L, char const* method, bool wantVertices) {
        auto& mesh = requireMesh(L, checkMeshHandle(L, 1, method), method);
        push(L, static_cast<long long>(wantVertices ? mesh->vertexCount() : mesh->primitiveCount()));
        return 1;
    }

    int meshVertexCount(lua_State* L) {
        return meshCountAxis(L, "Mesh:vertexCount", true);
    }

    int meshPrimitiveCount(lua_State* L) {
        return meshCountAxis(L, "Mesh:primitiveCount", false);
    }

    int meshBoundingBox(lua_State* L) {
        auto& mesh = requireMesh(L, checkMeshHandle(L, 1, "Mesh:boundingBox"), "Mesh:boundingBox");
        auto const& bounds = mesh->boundingBox();

        lua_createtable(L, 0, 3);
        pushVec3(L, bounds.min);
        lua_setfield(L, -2, "min");
        pushVec3(L, bounds.max);
        lua_setfield(L, -2, "max");
        push(L, bounds.empty);
        lua_setfield(L, -2, "empty");
        return 1;
    }

    int meshMaterialCount(lua_State* L) {
        auto& mesh =
            requireMesh(L, checkMeshHandle(L, 1, "Mesh:materialCount"), "Mesh:materialCount");
        push(L, static_cast<long long>(mesh->materialCount()));
        return 1;
    }

    int meshGetMaterial(lua_State* L) {
        auto& mesh = requireMesh(L, checkMeshHandle(L, 1, "Mesh:getMaterial"), "Mesh:getMaterial");
        int const index = check<int>(L, 2, "Mesh:getMaterial");
        if (index < 0 || static_cast<std::size_t>(index) >= mesh->materialCount()) {
            lua_pushnil(L);
            return 1;
        }

        auto const& data = mesh->materials()[static_cast<std::size_t>(index)];
        auto material = std::make_shared<Material>();
        material->baseColorFactor = data.baseColorFactor;
        material->imageIndex = data.imageIndex;
        material->alphaMode = data.alphaMode;
        material->alphaCutoff = data.alphaCutoff;
        material->doubleSided = data.doubleSided;
        material->sourceMesh = mesh;
        pushMaterial(L, std::move(material));
        return 1;
    }

    int meshGc(lua_State* L) {
        releaseMeshBox(checkMeshHandle(L, 1, "Mesh.__gc"));
        return 0;
    }

    void meshBoxDtor(lua_State* L, void* ud) {
        (void)L;
        releaseMeshBox(static_cast<MeshBox*>(ud));
        static_cast<MeshBox*>(ud)->~MeshBox();
    }
} // namespace

namespace luax::gd3d {
    void registerMeshHandleMetatable(lua_State* L) {
        luaL_Reg const methods[] = {
            {"vertexCount", meshVertexCount},
            {"primitiveCount", meshPrimitiveCount},
            {"boundingBox", meshBoundingBox},
            {"materialCount", meshMaterialCount},
            {"getMaterial", meshGetMaterial},
            {"__gc", meshGc},
            {nullptr, nullptr},
        };

        registerTaggedMetatable(
            L, kMeshMeta, luax::detail::meshAssetTag(), methods, std::nullopt, &meshBoxDtor, kMeshTypeName
        );
    }
} // namespace luax::gd3d
