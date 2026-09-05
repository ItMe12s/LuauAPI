#include "bindings/render3d/internal/Handles.hpp"
#include "framework/Binding.hpp"
#include "lua_test_helpers.hpp"
#include "render3d/assets/MeshAsset.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace luax {
    geode::Result<void> registerGd3d(lua_State* L);
} // namespace luax

namespace {
    using namespace luax;
    using namespace luax::gd3d;
    using namespace luax::render3d;
    using luauapi_test::BindingGuard;
    using luauapi_test::makeLuaState;
    using luauapi_test::runScriptPcall;
    using luauapi_test::runScriptReturnsBool;
    using luauapi_test::runScriptReturnsString;

    std::filesystem::path repoRoot() {
        return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
    }

    void registerGd3dBindings(lua_State* L) {
        registerBinding({"gd3d", &registerGd3d, 0});
        REQUIRE(applyAllBindings(L) == std::nullopt);
    }

    bool runScriptLeavesMeshOnStack(lua_State* L, std::string const& source) {
        if (!runScriptPcall(L, source)) {
            return false;
        }
        return lua_isuserdata(L, -1);
    }

    bool meshOnStackIsLive(lua_State* L) {
        auto* box = checkMeshHandle(L, -1, "test");
        return box->mesh != nullptr;
    }

    char const* kMinimalTriangleGltf =
        R"({"asset": {"version": "2.0"}, "materials": [{"pbrMetallicRoughness": {}}],
  "buffers": [{
    "byteLength": 42,
    "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA"
  }],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 6}
  ],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}
  ],
  "meshes": [{
    "primitives": [{
      "attributes": {"POSITION": 0},
      "indices": 1,
      "material": 0
    }]
  }],
  "nodes": [{"mesh": 0}],
  "scenes": [{"nodes": [0]}],
  "scene": 0
})";
} // namespace

TEST_CASE("gd3d.mesh.new builds triangle from 1-based indices") {
    BindingGuard guard;
    auto L = makeLuaState(true);
    registerGd3dBindings(L.get());

    REQUIRE(runScriptReturnsBool(L.get(), R"(
        local mesh = gd3d.mesh.new({
            positions = {
                { x = 0, y = 0, z = 0 },
                { x = 1, y = 0, z = 0 },
                { x = 0, y = 1, z = 0 },
            },
            indices = { 1, 2, 3 },
        })
        if not mesh then
            return false
        end
        return mesh:vertexCount() == 3 and mesh:primitiveCount() == 1
    )"));
}

TEST_CASE("gd3d.mesh.new rejects out-of-range 1-based index") {
    BindingGuard guard;
    auto L = makeLuaState(true);
    registerGd3dBindings(L.get());

    auto err = runScriptReturnsString(L.get(), R"(
        local mesh, err = gd3d.mesh.new({
            positions = {
                { x = 0, y = 0, z = 0 },
                { x = 1, y = 0, z = 0 },
                { x = 0, y = 1, z = 0 },
            },
            indices = { 1, 2, 4 },
        })
        return err
    )");
    REQUIRE(err.has_value());
    REQUIRE(err->find("index out of range") != std::string::npos);
}

TEST_CASE("gd3d.gltf.loadMeshFromBytes parses minimal embedded glTF") {
    BindingGuard guard;
    auto L = makeLuaState(true);
    registerGd3dBindings(L.get());

    lua_pushlstring(L.get(), kMinimalTriangleGltf, std::strlen(kMinimalTriangleGltf));
    lua_setglobal(L.get(), "minimal_gltf");

    REQUIRE(runScriptReturnsBool(L.get(), R"(
        local mesh, err = gd3d.gltf.loadMeshFromBytes(minimal_gltf)
        if not mesh then
            return false
        end
        return mesh:vertexCount() == 3 and mesh:primitiveCount() == 1
    )"));
}

// NOTE: donut bytes case lives on device only (ImagePlus not linked on host).

TEST_CASE("mesh handle __gc drops shared mesh") {
    BindingGuard guard;
    auto L = makeLuaState(true);
    registerGd3dBindings(L.get());

    REQUIRE(runScriptLeavesMeshOnStack(L.get(), R"(
        return gd3d.mesh.new({
            positions = {
                { x = 0, y = 0, z = 0 },
                { x = 1, y = 0, z = 0 },
                { x = 0, y = 1, z = 0 },
            },
            indices = { 1, 2, 3 },
        })
    )"));

    REQUIRE(meshOnStackIsLive(L.get()));

    lua_pop(L.get(), 1);
    lua_gc(L.get(), LUA_GCCOLLECT, 0);
}

TEST_CASE("gd3d.mesh.new rejects malformed Vec3 entries without raising") {
    BindingGuard guard;
    auto L = makeLuaState(true);
    registerGd3dBindings(L.get());

    auto err = runScriptReturnsString(L.get(), R"(
        local mesh, err = gd3d.mesh.new({
            positions = {
                { x = "bad", y = 0, z = 0 },
                { x = 0, y = 1, z = 0 },
                { x = 0, y = 1, z = 0 },
            },
            indices = { 1, 2, 3 },
        })
        return err
    )");
    REQUIRE(err.has_value());
    REQUIRE(err->find("positions entries must be Vec3 tables") != std::string::npos);

    REQUIRE(runScriptReturnsBool(L.get(), R"(
        local mesh = gd3d.mesh.new({
            positions = {
                { x = 0, y = 0, z = 0 },
                { x = 1, y = 0, z = 0 },
                { x = 0, y = 1, z = 0 },
            },
            indices = { 1, 2, 3 },
        })
        return mesh ~= nil
    )"));
}

TEST_CASE("gd3d.mesh.new rejects malformed UV entries without raising") {
    BindingGuard guard;
    auto L = makeLuaState(true);
    registerGd3dBindings(L.get());

    auto err = runScriptReturnsString(L.get(), R"(
        local mesh, err = gd3d.mesh.new({
            positions = {
                { x = 0, y = 0, z = 0 },
                { x = 1, y = 0, z = 0 },
                { x = 0, y = 1, z = 0 },
            },
            uvs = { { x = 0 } },
            indices = { 1, 2, 3 },
        })
        return err
    )");
    REQUIRE(err.has_value());
    REQUIRE(err->find("uvs entries must be { x, y } tables") != std::string::npos);
}
