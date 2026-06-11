#include "bindings/geode/ModSandbox.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/MeshAsset.hpp"

#include <Geode/Geode.hpp>
#include <filesystem>
#include <glm/vec3.hpp>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;
    using namespace luax::render3d;

    constexpr char const* kMeshMeta = "luax.gd3d.Mesh";
    constexpr char const* kMeshTypeName = "Mesh";

    struct MeshHandle {
        std::uint64_t id = 0;
    };

    void pushVec3(lua_State* L, glm::vec3 const& v) {
        lua_createtable(L, 0, 3);
        lua_pushnumber(L, v.x);
        lua_setfield(L, -2, "x");
        lua_pushnumber(L, v.y);
        lua_setfield(L, -2, "y");
        lua_pushnumber(L, v.z);
        lua_setfield(L, -2, "z");
    }

    void releaseMeshHandle(MeshHandle* handle) {
        if (handle == nullptr || handle->id == 0) {
            return;
        }
        MeshRegistry::instance().release(handle->id);
        handle->id = 0;
    }

    void pushMeshHandle(lua_State* L, std::uint64_t id) {
        auto* handle = static_cast<MeshHandle*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(MeshHandle), detail::meshAssetTag())
        );
        handle->id = id;
    }

    MeshHandle* checkMeshHandle(lua_State* L, int idx, [[maybe_unused]] char const* method) {
        return static_cast<MeshHandle*>(luaL_checkudata(L, idx, kMeshMeta));
    }

    std::shared_ptr<MeshAsset> requireMesh(lua_State* L, MeshHandle* handle, char const* method) {
        if (handle == nullptr || handle->id == 0) {
            luaL_error(L, "%s: mesh handle is invalid", method);
        }

        auto mesh = MeshRegistry::instance().get(handle->id);
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

        lua_getuserdatametatable(L, detail::meshAssetTag());
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);

        luaL_getmetatable(L, kMeshMeta);
        lua_setuserdatametatable(L, detail::meshAssetTag());
        lua_setuserdatadtor(L, detail::meshAssetTag(), &meshHandleDtor);
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
} // namespace

namespace luax {
    geode::Result<void> registerGltf(lua_State* L) {
        registerMeshHandleMetatable(L);

        getOrCreateTable(L, "gd3d.gltf");
        setTableCFunction(L, -1, "loadMesh", &gltfLoadMesh);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(gd3d_gltf_lib, registerGltf)
#endif
