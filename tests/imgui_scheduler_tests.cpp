#include "host/lua_test_helpers.hpp"
#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "core/Runtime.hpp"

#include <RuntimeTypes.hpp>
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
} // namespace

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

TEST_CASE(
    "ImGuiDrawScheduler drawAll restores stack when protectedCall fails "
    "early"
) {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    int topBefore = lua_gettop(L);
    auto ref = luauapi_test::makeCallback(L, "return");
    auto& scheduler = luax::ImGuiDrawScheduler::get();
    auto id = scheduler.add(std::move(ref));
    REQUIRE(id != 0);

    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::NotReady);
    scheduler.drawAll();
    REQUIRE(lua_gettop(L) == topBefore);
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

TEST_CASE("ImGuiDrawScheduler m_slots stays valid after swap-and-pop compaction") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto refHead = luauapi_test::makeCallback(L, "_G.drawHead = (_G.drawHead or 0) + 1");
    auto refMid = luauapi_test::makeCallback(L, "_G.drawMid = (_G.drawMid or 0) + 1");
    auto refTail = luauapi_test::makeCallback(L, "_G.drawTail = (_G.drawTail or 0) + 1");

    auto& scheduler = luax::ImGuiDrawScheduler::get();
    auto headId = scheduler.add(std::move(refHead));
    auto midId = scheduler.add(std::move(refMid));
    auto tailId = scheduler.add(std::move(refTail));
    REQUIRE(headId != 0);
    REQUIRE(midId != 0);
    REQUIRE(tailId != 0);

    scheduler.cancel(midId);
    scheduler.drawAll();

    REQUIRE(scheduler.activeCount() == 2);

    scheduler.drawAll();

    lua_getglobal(L, "drawHead");
    REQUIRE(lua_tointeger(L, -1) == 2);
    lua_pop(L, 1);
    lua_getglobal(L, "drawMid");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
    lua_getglobal(L, "drawTail");
    REQUIRE(lua_tointeger(L, -1) == 2);
    lua_pop(L, 1);
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
