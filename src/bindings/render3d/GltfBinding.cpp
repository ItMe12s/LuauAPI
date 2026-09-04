#include "bindings/geode/ModSandbox.hpp"
#include "bindings/render3d/internal/Marshaling.hpp"
#include "bindings/render3d/internal/MeshHandleBinding.hpp"
#include "core/Config.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "render3d/assets/MeshAsset.hpp"

#include <Geode/Geode.hpp>
#include <Geode/Result.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <span>

namespace {
    using namespace luax;
    using namespace luax::gd3d;
    using namespace luax::render3d;

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
        if (auto err = returnIfErr(L, result)) {
            return *err;
        }

        pushMeshHandle(L, std::move(result.unwrap()));
        return 1;
    }

    int gltfLoadMeshFromBytes(lua_State* L) {
        auto bytesResult = checkBufferOrString(
            L,
            1,
            "gd3d.gltf.loadMeshFromBytes",
            kMaxFsReadBytes,
            "glTF data is empty",
            "glTF data exceeds maximum read size"
        );
        if (auto err = returnIfErr(L, bytesResult)) {
            return *err;
        }

        auto result = MeshAsset::loadFromBytes(
            bytesResult.unwrap(), std::filesystem::path{}, std::filesystem::path{}
        );
        if (auto err = returnIfErr(L, result)) {
            return *err;
        }

        pushMeshHandle(L, std::move(result.unwrap()));
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
