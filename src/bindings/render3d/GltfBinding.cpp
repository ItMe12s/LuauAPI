#include "bindings/geode/ModSandbox.hpp"
#include "bindings/render3d/Gd3dShared.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/assets/MeshAsset.hpp"
#include "render3d/gpu/Renderer3D.hpp"
#include "render3d/types/Material.hpp"

#include <Geode/Geode.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/vec3.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <span>

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
        Renderer3D::instance().releaseMeshGpu(id);
        handle->id = 0;
    }

    void pushMeshHandle(lua_State* L, std::uint64_t id) {
        auto* handle = static_cast<MeshHandle*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(MeshHandle), luax::detail::meshAssetTag())
        );
        handle->id = id;
    }

    std::shared_ptr<MeshAsset> requireMesh(lua_State* L, MeshHandle* handle, char const* method) {
        auto mesh = MeshRegistry::instance().get(requireMeshId(L, handle, method));
        if (!mesh) {
            luaL_error(L, "%s: mesh handle is invalid", method);
        }
        return mesh;
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

        if (luaL_newmetatable(L, kMeshMeta)) {
            for (luaL_Reg const* reg = methods; reg->name != nullptr; ++reg) {
                setTableCFunction(L, -1, reg->name, reg->func);
            }
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            lua_pushstring(L, kMeshTypeName);
            lua_setfield(L, -2, "__type");
        }
        lua_pop(L, 1);

        lua_getuserdatametatable(L, luax::detail::meshAssetTag());
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);

        luaL_getmetatable(L, kMeshMeta);
        lua_setuserdatametatable(L, luax::detail::meshAssetTag());
        lua_setuserdatadtor(L, luax::detail::meshAssetTag(), &meshHandleDtor);
    }

    LoadResult<std::span<std::uint8_t const>> readGltfBytesArg(lua_State* L, int idx) {
        if (lua_isbuffer(L, idx)) {
            size_t len = 0;
            void* data = lua_tobuffer(L, idx, &len);
            if (data == nullptr || len == 0) {
                return LoadResult<std::span<std::uint8_t const>>::err("glTF data is empty");
            }
            if (len > kMaxFsReadBytes) {
                return LoadResult<std::span<std::uint8_t const>>::err(
                    "glTF data exceeds maximum read size"
                );
            }
            return LoadResult<std::span<std::uint8_t const>>::ok(
                std::span<std::uint8_t const>(static_cast<std::uint8_t const*>(data), len)
            );
        }

        size_t len = 0;
        char const* text = lua_tolstring(L, idx, &len);
        if (text == nullptr) {
            luaL_error(L, "gd3d.gltf.loadMeshFromBytes expected buffer or string at arg %d", idx);
        }
        if (len == 0) {
            return LoadResult<std::span<std::uint8_t const>>::err("glTF data is empty");
        }
        if (len > kMaxFsReadBytes) {
            return LoadResult<std::span<std::uint8_t const>>::err(
                "glTF data exceeds maximum read size"
            );
        }
        return LoadResult<std::span<std::uint8_t const>>::ok(
            std::span<std::uint8_t const>(reinterpret_cast<std::uint8_t const*>(text), len)
        );
    }

    int gltfLoadMesh(lua_State* L) {
        auto target = resolveSandboxTarget(L, 1, 2, "gd3d.gltf.loadMesh");
        if (!target) {
            return 2;
        }

        std::error_code ec;
        if (!std::filesystem::is_regular_file(target->path, ec)) {
            return pushNilErr(L, "path is not a regular file");
        }

        auto result = MeshAsset::loadFromFile(target->path);
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }

        auto const id = MeshRegistry::instance().registerMesh(result.unwrap());
        pushMeshHandle(L, id);
        return 1;
    }

    int gltfLoadMeshFromBytes(lua_State* L) {
        auto bytesResult = readGltfBytesArg(L, 1);
        if (bytesResult.isErr()) {
            return pushNilErr(L, bytesResult.unwrapErr());
        }

        auto result = MeshAsset::loadFromBytes(
            bytesResult.unwrap(), std::filesystem::path{}, std::filesystem::path{}
        );
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }

        auto const id = MeshRegistry::instance().registerMesh(result.unwrap());
        pushMeshHandle(L, id);
        return 1;
    }
} // namespace

namespace luax {
    geode::Result<void> registerGltf(lua_State* L) {
        registerMeshHandleMetatable(L);

        getOrCreateTable(L, "gd3d.gltf");
        setTableCFunction(L, -1, "loadMesh", &gltfLoadMesh);
        setTableCFunction(L, -1, "loadMeshFromBytes", &gltfLoadMeshFromBytes);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(gd3d_gltf_lib, registerGltf)
#endif
