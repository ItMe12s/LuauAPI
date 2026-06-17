#include "bindings/geode/ModSandbox.hpp"
#include "bindings/render3d/internal/MeshHandleBinding.hpp"
#include "core/Config.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "render3d/assets/MeshAsset.hpp"

#include <Geode/Geode.hpp>
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

    std::expected<std::span<std::uint8_t const>, std::string> readGltfBytesArg(lua_State* L, int idx) {
        if (lua_isbuffer(L, idx)) {
            size_t len = 0;
            void* data = lua_tobuffer(L, idx, &len);
            if (data == nullptr || len == 0) {
                return std::unexpected("glTF data is empty");
            }
            if (len > kMaxFsReadBytes) {
                return std::unexpected("glTF data exceeds maximum read size");
            }
            return std::span<std::uint8_t const>(static_cast<std::uint8_t const*>(data), len);
        }

        size_t len = 0;
        char const* text = lua_tolstring(L, idx, &len);
        if (text == nullptr) {
            luaL_error(L, "gd3d.gltf.loadMeshFromBytes expected buffer or string at arg %d", idx);
        }
        if (len == 0) {
            return std::unexpected("glTF data is empty");
        }
        if (len > kMaxFsReadBytes) {
            return std::unexpected("glTF data exceeds maximum read size");
        }
        return std::span<std::uint8_t const>(reinterpret_cast<std::uint8_t const*>(text), len);
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
        if (!result.has_value()) {
            return pushNilErr(L, result.error());
        }

        auto const id = MeshRegistry::instance().registerMesh(std::move(result).value());
        pushMeshHandle(L, id);
        return 1;
    }

    int gltfLoadMeshFromBytes(lua_State* L) {
        auto bytesResult = readGltfBytesArg(L, 1);
        if (!bytesResult.has_value()) {
            return pushNilErr(L, bytesResult.error());
        }

        auto result = MeshAsset::loadFromBytes(
            bytesResult.value(), std::filesystem::path{}, std::filesystem::path{}
        );
        if (!result.has_value()) {
            return pushNilErr(L, result.error());
        }

        auto const id = MeshRegistry::instance().registerMesh(std::move(result).value());
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
