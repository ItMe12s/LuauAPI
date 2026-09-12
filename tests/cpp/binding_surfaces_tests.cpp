#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "bindings/task/TaskScheduler.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "host/ImGuiTestHarness.hpp"
#include "host/lua_test_helpers.hpp"
#include "require/Requirer.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/utils/web.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <lua.h>
#include <string>
#include <thread>

namespace luax {
    geode::Result<void> registerGd3d(lua_State* L);
    geode::Result<void> registerGeodeWeb(lua_State* L);
    geode::Result<void> registerImGui(lua_State* L);
    geode::Result<void> registerTask(lua_State* L);
    geode::Result<void> registerWebSocket(lua_State* L);
} // namespace luax

namespace {
    using namespace luax;
    using Clock = std::chrono::steady_clock;

    struct SurfaceGuard {
        SurfaceGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            geode::test::bindMainThreadToCurrent();
            luax::resetBindingsForTests();
        }

        ~SurfaceGuard() {
            luax::TaskScheduler::get().clear();
            luax::ImGuiDrawScheduler::get().clear();
            luax::clearWsState();
            geode::test::clearMainThreadQueue();
            geode::utils::web::test::resetResponseFactory();
            geode::utils::web::test::resetSendCount();
            luax::invalidateCurrentModCache();
            geode::Mod::resetForTests();
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };

    struct ImGuiContextGuard : luauapi_test::ImGuiTestContext {};

    bool globalIsTrue(lua_State* L, char const* name) {
        lua_getglobal(L, name);
        bool const value = lua_isboolean(L, -1) && lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return value;
    }

    bool waitUntil(lua_State* L, std::function<bool()> pred, int timeoutMs = 5000) {
        auto const deadline = Clock::now() + std::chrono::milliseconds(timeoutMs);
        while (Clock::now() < deadline) {
            geode::test::drainMainThreadQueue();
            if (pred()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        geode::test::drainMainThreadQueue();
        return pred();
    }

    struct LoadModuleCtx {
        Requirer* req;
        std::string chunkname;
        std::string loadname;
    };

    int callLoadModule(lua_State* L) {
        auto* ctx = static_cast<LoadModuleCtx*>(lua_tolightuserdata(L, lua_upvalueindex(1)));
        return ctx->req->loadModule(L, ctx->chunkname.c_str(), ctx->loadname.c_str());
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

TEST_CASE("binding surfaces: print, require, task, web, websocket, imgui, glTF") {
    SurfaceGuard guard;
    ImGuiContextGuard ctx;
    luauapi_test::ScopedTempDir root{"luauapi_bindings_"};
    luauapi_test::writeTestFile(root.path / "HelloModule.luau", "return 42");

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(root.path);
    lua_State* L = runtime->state();
    REQUIRE(L != nullptr);

    geode::Mod* mod = geode::Mod::create(root.path);
    REQUIRE(mod != nullptr);

    luax::registerBinding({"task_lib", &luax::registerTask, 10});
    luax::registerBinding({"imgui_lib", &luax::registerImGui, 10});
    luax::registerBinding({"geode_web", &registerGeodeWeb, 0});
    luax::registerBinding({"gd3d", &registerGd3d, 0});
    REQUIRE(luax::applyAllBindings(L) == std::nullopt);
    REQUIRE(registerWebSocket(L).isOk());

    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(
        print("print works")
        return true
    )"));

    {
        Requirer req(*runtime);
        req.setResourcesRoot(root.path);
        REQUIRE(req.toChild("HelloModule") == NAVIGATE_SUCCESS);

        char cacheKey[1024];
        size_t cacheKeySize = 0;
        REQUIRE(req.writeCacheKey(cacheKey, sizeof(cacheKey), &cacheKeySize) == WRITE_SUCCESS);

        auto chunk = req.chunkname();
        LoadModuleCtx ctx{&req, chunk, chunk};

        int base = lua_gettop(L);
        lua_pushlightuserdata(L, &ctx);
        lua_pushcclosure(L, callLoadModule, "callLoadModule", 1);
        REQUIRE(lua_pcall(L, 0, LUA_MULTRET, 0) == 0);
        REQUIRE(lua_gettop(L) - base == 1);
        REQUIRE(lua_isnumber(L, -1));
        REQUIRE(lua_tonumber(L, -1) == 42.0);
        lua_settop(L, base);
    }

    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(
        _G.taskHits = 0
        _G.taskHandle = task.defer(function() _G.taskHits = _G.taskHits + 1 end)
        return type(_G.taskHandle) == "userdata"
    )"));
    TaskScheduler::get().advance(0.0);
    REQUIRE(luauapi_test::runScriptReturnsBool(L, "return _G.taskHits == 1"));

    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(
        local ok = false
        geode.utils.web.get("http://example.test", function(res, err)
            ok = res ~= nil and err == nil and res:ok() and res:text() == "OK"
        end)
        return ok
    )"));

    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(
        _ws_server = websocket.serve(59142)
        if not _ws_server then
            return false
        end
        _ws_server:onMessage(function(peer, data, isBinary)
            peer:send(data)
        end)
        return true
    )"));
    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(
        _ws_client = websocket.connect("ws://127.0.0.1:59142")
        _ws_client:onOpen(function()
            _ws_client:send("hello")
        end)
        _ws_client:onMessage(function(data, isBinary)
            _G.wsEcho = data
        end)
        return true
    )"));
    REQUIRE(waitUntil(L, [&] {
        return luauapi_test::runScriptReturnsBool(L, "return _G.wsEcho == 'hello'");
    }));

    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(
        _G.imguiDrew = false
        _G.imguiHandle = imgui.onDraw(function()
            imgui.text("hello")
            _G.imguiDrew = true
        end)
        return type(_G.imguiHandle) == "userdata"
    )"));
    luauapi_test::beginImGuiTestFrame();
    ImGuiDrawScheduler::get().drawAll();
    luauapi_test::endImGuiTestFrame();
    REQUIRE(globalIsTrue(L, "imguiDrew"));

    lua_pushlstring(L, kMinimalTriangleGltf, std::strlen(kMinimalTriangleGltf));
    lua_setglobal(L, "gltf_bytes");
    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(
        local mesh, err = gd3d.gltf.loadMeshFromBytes(gltf_bytes)
        if not mesh then
            return false
        end
        return mesh:vertexCount() == 3 and mesh:primitiveCount() == 1
    )"));

    geode::Mod::destroy(mod);
}