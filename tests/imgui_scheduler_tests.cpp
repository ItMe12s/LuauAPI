#include "lua/bindings/imgui/ImGuiDrawScheduler.hpp"
#include "lua/runtime/Runtime.hpp"
#include "lua_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>

#include <thread>

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::ImGuiDrawScheduler::get().clear();
            luax::Runtime::resetForTests();
        }
    };
}

TEST_CASE("ImGuiDrawScheduler fires registered draw callbacks") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, R"(
        _G.drawHits = (_G.drawHits or 0) + 1
    )");

    auto& scheduler = luax::ImGuiDrawScheduler::get();
    auto id = scheduler.add(std::move(ref));
    REQUIRE(id != 0);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.drawAll();
    REQUIRE(scheduler.activeCount() == 1);
    REQUIRE_FALSE(scheduler.inFrame());

    lua_getglobal(L, "drawHits");
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);

    scheduler.clear();
}

TEST_CASE("ImGuiDrawScheduler cancels callbacks that error") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "error('draw failed')");

    auto& scheduler = luax::ImGuiDrawScheduler::get();
    auto id = scheduler.add(std::move(ref));
    REQUIRE(id != 0);

    scheduler.drawAll();
    REQUIRE(scheduler.activeCount() == 0);
}

TEST_CASE("ImGuiDrawScheduler honors cancel before draw") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "_G.cancelledDraw = (_G.cancelledDraw or 0) + 1");

    auto& scheduler = luax::ImGuiDrawScheduler::get();
    auto id = scheduler.add(std::move(ref));
    scheduler.cancel(id);
    scheduler.drawAll();

    REQUIRE(scheduler.activeCount() == 0);
    lua_getglobal(L, "cancelledDraw");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}
