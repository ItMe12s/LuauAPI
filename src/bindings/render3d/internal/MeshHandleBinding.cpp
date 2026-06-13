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

    void releaseMeshHandle(MeshHandle* handle) {
        if (handle == nullptr || handle->id == 0) {
            return;
        }
        std::uint64_t const id = handle->id;
        MeshRegistry::instance().release(id);
#if !defined(LUAUAPI_HOST_TESTS)
        Renderer3D::instance().releaseMeshGpu(id);
#endif
        handle->id = 0;
    }

    int meshVertexCount(lua_State* L) {
        auto* handle = checkMeshHandle(L, 1, "Mesh:vertexCount");
        auto mesh = requireMesh(L, handle, "Mesh:vertexCount");
        push(L, static_cast<long long>(mesh->vertexCount()));
        return 1;
    }

    int meshPrimitiveCount(lua_State* L) {
        auto* handle = checkMeshHandle(L, 1, "Mesh:primitiveCount");
        auto mesh = requireMesh(L, handle, "Mesh:primitiveCount");
        push(L, static_cast<long long>(mesh->primitiveCount()));
        return 1;
    }

    int meshBoundingBox(lua_State* L) {
        auto* handle = checkMeshHandle(L, 1, "Mesh:boundingBox");
        auto mesh = requireMesh(L, handle, "Mesh:boundingBox");
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
        auto* handle = checkMeshHandle(L, 1, "Mesh:materialCount");
        auto mesh = requireMesh(L, handle, "Mesh:materialCount");
        push(L, static_cast<long long>(mesh->materialCount()));
        return 1;
    }

    int meshGetMaterial(lua_State* L) {
        auto* handle = checkMeshHandle(L, 1, "Mesh:getMaterial");
        auto mesh = requireMesh(L, handle, "Mesh:getMaterial");
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
        material->sourceMeshId = requireMeshId(L, handle, "Mesh:getMaterial");
        pushMaterial(L, std::move(material));
        return 1;
    }

    int meshGc(lua_State* L) {
        releaseMeshHandle(checkMeshHandle(L, 1, "Mesh.__gc"));
        return 0;
    }

    void meshHandleDtor(lua_State* L, void* ud) {
        (void)L;
        releaseMeshHandle(static_cast<MeshHandle*>(ud));
    }
} // namespace

namespace luax::gd3d {
    void pushMeshHandle(lua_State* L, std::uint64_t id) {
        auto* handle = static_cast<MeshHandle*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(MeshHandle), luax::detail::meshAssetTag())
        );
        handle->id = id;
    }

    std::shared_ptr<render3d::MeshAsset> requireMesh(lua_State* L, MeshHandle* handle, char const* method) {
        auto mesh = render3d::MeshRegistry::instance().get(requireMeshId(L, handle, method));
        if (!mesh) {
            luaL_error(L, "%s: mesh handle is invalid", method);
        }
        return mesh;
    }

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
            L, kMeshMeta, luax::detail::meshAssetTag(), methods, std::nullopt, &meshHandleDtor, kMeshTypeName
        );
    }
} // namespace luax::gd3d
