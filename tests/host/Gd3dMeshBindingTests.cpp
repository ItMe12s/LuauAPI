#include "bindings/render3d/internal/Handles.hpp"
#include "framework/Binding.hpp"
#include "render3d/assets/MeshAsset.hpp"

#include <Luau/Compiler.h>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <lua.h>
#include <lualib.h>
#include <memory>
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

    struct LuaStateDeleter {
        void operator()(lua_State* L) const {
            if (L) {
                lua_close(L);
            }
        }
    };

    using LuaStatePtr = std::unique_ptr<lua_State, LuaStateDeleter>;

    struct BindingGuard {
        BindingGuard() {
            resetBindingsForTests();
        }

        ~BindingGuard() {
            resetBindingsForTests();
        }
    };

    std::filesystem::path repoRoot() {
        return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    }

    LuaStatePtr makeLuaState() {
        auto* L = luaL_newstate();
        REQUIRE(L != nullptr);
        luaL_openlibs(L);
        return LuaStatePtr(L);
    }

    std::string compile(std::string const& source) {
        Luau::CompileOptions opts;
        opts.optimizationLevel = 2;
        opts.debugLevel = 1;
        opts.typeInfoLevel = 1;
        return Luau::compile(source, opts);
    }

    void registerGd3dBindings(lua_State* L) {
        registerBinding({"gd3d", &registerGd3d, 0});
        REQUIRE(applyAllBindings(L) == std::nullopt);
    }

    bool runScriptReturnsBool(lua_State* L, std::string const& source) {
        auto bytecode = compile(source);
        if (luau_load(L, "=gd3d_mesh_test", bytecode.data(), bytecode.size(), 0) != 0) {
            return false;
        }
        if (lua_pcall(L, 0, 1, 0) != 0) {
            return false;
        }
        if (!lua_isboolean(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        bool const value = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return value;
    }

    std::optional<std::string> runScriptReturnsString(lua_State* L, std::string const& source) {
        auto bytecode = compile(source);
        if (luau_load(L, "=gd3d_mesh_test", bytecode.data(), bytecode.size(), 0) != 0) {
            return std::nullopt;
        }
        if (lua_pcall(L, 0, 1, 0) != 0) {
            return std::nullopt;
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        size_t len = 0;
        char const* text = lua_tolstring(L, -1, &len);
        std::string value(text ? text : "", len);
        lua_pop(L, 1);
        return value;
    }

    bool runScriptLeavesMeshOnStack(lua_State* L, std::string const& source) {
        auto bytecode = compile(source);
        if (luau_load(L, "=gd3d_mesh_test", bytecode.data(), bytecode.size(), 0) != 0) {
            return false;
        }
        if (lua_pcall(L, 0, 1, 0) != 0) {
            return false;
        }
        return lua_isuserdata(L, -1);
    }

    std::uint64_t meshIdFromStack(lua_State* L) {
        auto* handle = checkMeshHandle(L, -1, "test");
        return handle->id;
    }

    char const* kMinimalTriangleGltf = R"({"asset": {"version": "2.0"}, "materials": [{"pbrMetallicRoughness": {}}],
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
    auto L = makeLuaState();
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
    auto L = makeLuaState();
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
    auto L = makeLuaState();
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

TEST_CASE("gd3d.gltf.loadMeshFromBytes parses test_donut.glb bytes") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    auto const path = repoRoot() / "resources" / "test_donut.glb";
    INFO(path);
    REQUIRE(std::filesystem::exists(path));

    std::ifstream input(path, std::ios::binary);
    REQUIRE(input.good());
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    REQUIRE_FALSE(bytes.empty());

    lua_pushlstring(L.get(), bytes.data(), bytes.size());
    lua_setglobal(L.get(), "donut_glb_bytes");

    REQUIRE(runScriptReturnsBool(L.get(), R"(
        local mesh, err = gd3d.gltf.loadMeshFromBytes(donut_glb_bytes)
        if not mesh then
            return false
        end
        return mesh:vertexCount() > 0 and mesh:primitiveCount() > 0
    )"));
}

TEST_CASE("mesh handle __gc releases MeshRegistry entry") {
    BindingGuard guard;
    auto L = makeLuaState();
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

    std::uint64_t const id = meshIdFromStack(L.get());
    REQUIRE(id != 0);
    REQUIRE(MeshRegistry::instance().get(id) != nullptr);

    lua_pop(L.get(), 1);
    lua_gc(L.get(), LUA_GCCOLLECT, 0);

    REQUIRE(MeshRegistry::instance().get(id) == nullptr);
}
