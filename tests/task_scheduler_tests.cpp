#include "lua/bindings/task/TaskScheduler.hpp"
#include "lua/runtime/Runtime.hpp"
#include "lua_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>

#include <atomic>
#include <thread>

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::TaskScheduler::get().clear();
            luax::Runtime::resetForTests();
        }
    };
}

TEST_CASE("TaskScheduler fires one-shot tasks after delay") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    std::atomic<int> hits{0};
    lua_pushinteger(L, hits.load());
    lua_setglobal(L, "hits");

    auto ref = luauapi_test::makeCallback(L, R"(
        local hits = _G.hits or 0
        _G.hits = hits + 1
    )");

    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.add(std::move(ref), 0.1, 0.0);
    REQUIRE(id != 0);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.advance(0.05, L);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.advance(0.1, L);
    REQUIRE(scheduler.activeCount() == 0);

    lua_getglobal(L, "hits");
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);
}

TEST_CASE("TaskScheduler repeats interval tasks until cancelled") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "_G.intervalHits = (_G.intervalHits or 0) + 1");

    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.add(std::move(ref), 0.0, 0.5);
    REQUIRE(id != 0);

    scheduler.advance(0.5, L);
    scheduler.advance(0.5, L);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.cancel(id);
    scheduler.advance(0.5, L);
    REQUIRE(scheduler.activeCount() == 0);

    lua_getglobal(L, "intervalHits");
    REQUIRE(lua_tointeger(L, -1) == 2);
    lua_pop(L, 1);
}

TEST_CASE("TaskScheduler cancels tasks that error") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "error('task failed')");

    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.add(std::move(ref), 0.0, 0.0);
    REQUIRE(id != 0);

    scheduler.advance(0.0, L);
    REQUIRE(scheduler.activeCount() == 0);
}
