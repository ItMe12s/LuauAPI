#include "host/lua_test_helpers.hpp"
#include "lua/bindings/Binding.hpp"
#include "lua/bindings/imgui/ImGuiDrawHandleBinding.hpp"
#include "lua/bindings/imgui/ImGuiDrawScheduler.hpp"
#include "lua/bindings/task/TaskScheduler.hpp"
#include "lua/runtime/Runtime.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <thread>

namespace luax {
    geode::Result<void> registerTask(lua_State* L);
}

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::resetBindingsForTests();
        }

        ~RuntimeGuard() {
            luax::ImGuiDrawScheduler::get().clear();
            luax::TaskScheduler::get().clear();
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };

    void registerTaskBinding(lua_State* L) {
        luax::registerBinding({"task_lib", &luax::registerTask, 10});
        REQUIRE(luax::applyAllBindings(L) == std::nullopt);
    }

    void registerImGuiDrawHandleBinding(lua_State* L) {
        luax::registerBinding({"imgui_draw", &luax::registerImGuiDrawHandle, 10});
        REQUIRE(luax::applyAllBindings(L) == std::nullopt);
    }

    std::uint64_t taskHandleId(lua_State* L, int idx) {
        void* userdata = luaL_checkudata(L, idx, "luax.TaskHandle");
        return *static_cast<std::uint64_t*>(userdata);
    }

    std::uint64_t imguiDrawHandleId(lua_State* L, int idx) {
        struct ImGuiDrawHandle {
            std::uint64_t id;
        };

        auto* handle = static_cast<ImGuiDrawHandle*>(luaL_checkudata(L, idx, "luax.ImGuiDrawHandle"));
        return handle->id;
    }
} // namespace

TEST_CASE("Task handle __gc cancels scheduled deferred task") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerTaskBinding(L);

    lua_getglobal(L, "task");
    lua_getfield(L, -1, "defer");
    luauapi_test::loadFunction(L, "_G.gcHits = (_G.gcHits or 0) + 1");
    REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
    REQUIRE(lua_isuserdata(L, -1));

    auto& scheduler = luax::TaskScheduler::get();
    auto id = taskHandleId(L, -1);
    REQUIRE(scheduler.isScheduled(id));
    REQUIRE(scheduler.activeCount() == 1);

    lua_pop(L, 1);
    lua_gc(L, LUA_GCCOLLECT, 0);

    REQUIRE_FALSE(scheduler.isScheduled(id));
    REQUIRE(scheduler.activeCount() == 0);

    scheduler.advance(0.0, L);
    lua_getglobal(L, "gcHits");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

TEST_CASE("Task handle __gc cancels scheduled interval task") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerTaskBinding(L);

    lua_getglobal(L, "task");
    lua_getfield(L, -1, "every");
    lua_pushnumber(L, 0.5);
    luauapi_test::loadFunction(L, "_G.intervalGcHits = (_G.intervalGcHits or 0) + 1");
    REQUIRE(lua_pcall(L, 2, 1, 0) == 0);
    REQUIRE(lua_isuserdata(L, -1));

    auto& scheduler = luax::TaskScheduler::get();
    auto id = taskHandleId(L, -1);
    REQUIRE(scheduler.isScheduled(id));

    lua_pop(L, 1);
    lua_gc(L, LUA_GCCOLLECT, 0);

    REQUIRE_FALSE(scheduler.isScheduled(id));
    REQUIRE(scheduler.activeCount() == 0);

    scheduler.advance(1.0, L);
    lua_getglobal(L, "intervalGcHits");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

TEST_CASE("ImGui draw handle __gc cancels scheduled draw callback") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiDrawHandleBinding(L);

    lua_getglobal(L, "imgui");
    lua_getfield(L, -1, "onDraw");
    luauapi_test::loadFunction(L, "_G.imguiGcHits = (_G.imguiGcHits or 0) + 1");
    REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
    REQUIRE(lua_isuserdata(L, -1));

    auto& scheduler = luax::ImGuiDrawScheduler::get();
    auto id = imguiDrawHandleId(L, -1);
    REQUIRE(id != 0);
    REQUIRE(scheduler.isScheduled(id));
    REQUIRE(scheduler.activeCount() == 1);

    lua_pop(L, 1);
    lua_gc(L, LUA_GCCOLLECT, 0);

    REQUIRE_FALSE(scheduler.isScheduled(id));
    REQUIRE(scheduler.activeCount() == 0);

    scheduler.drawAll();
    lua_getglobal(L, "imguiGcHits");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

TEST_CASE("ImGui draw handle cancellation prevents stale callback from drawing") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "_G.imguiGcHits = (_G.imguiGcHits or 0) + 1");
    auto& scheduler = luax::ImGuiDrawScheduler::get();
    auto id = scheduler.add(std::move(ref));
    REQUIRE(id != 0);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.cancel(id);
    lua_gc(L, LUA_GCCOLLECT, 0);
    scheduler.drawAll();

    REQUIRE(scheduler.activeCount() == 0);
    lua_getglobal(L, "imguiGcHits");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}
