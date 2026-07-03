#include "bindings/task/TaskScheduler.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "host/lua_test_helpers.hpp"

#include <RuntimeTypes.hpp>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>
#include <thread>
#include <vector>

namespace {
    using RuntimeGuard = luauapi_test::TaskSchedulerRuntimeGuard;
} // namespace

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

TEST_CASE("TaskScheduler advance restores stack when protectedCall fails early") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    int topBefore = lua_gettop(L);
    auto ref = luauapi_test::makeCallback(L, "return");
    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.add(std::move(ref), 0.0, 0.0);
    REQUIRE(id != 0);

    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::NotReady);
    scheduler.advance(0.0, L);
    REQUIRE(lua_gettop(L) == topBefore);
}

TEST_CASE(
    "TaskScheduler interval task drop-tick fires once after large frame "
    "delta"
) {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    lua_pushinteger(L, 0);
    lua_setglobal(L, "bigDtHits");

    auto ref = luauapi_test::makeCallback(L, "_G.bigDtHits = (_G.bigDtHits or 0) + 1");

    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.add(std::move(ref), 0.0, 0.5);
    REQUIRE(id != 0);

    scheduler.advance(2.0, L);
    REQUIRE(scheduler.activeCount() == 1);

    lua_getglobal(L, "bigDtHits");
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);
}

TEST_CASE("TaskScheduler defer fires on the next advance") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "_G.deferHits = (_G.deferHits or 0) + 1");

    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.addDeferred(std::move(ref));
    REQUIRE(id != 0);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.advance(0.0, L);
    REQUIRE(scheduler.activeCount() == 0);

    lua_getglobal(L, "deferHits");
    REQUIRE(lua_tointeger(L, -1) == 1);
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

TEST_CASE("TaskScheduler m_index stays valid after timed swap-and-pop compaction") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto refHead = luauapi_test::makeCallback(L, "_G.headHit = (_G.headHit or 0) + 1");
    auto refMid = luauapi_test::makeCallback(L, "_G.midHit = (_G.midHit or 0) + 1");
    auto refTail = luauapi_test::makeCallback(L, "_G.tailHit = (_G.tailHit or 0) + 1");

    auto& scheduler = luax::TaskScheduler::get();
    auto headId = scheduler.add(std::move(refHead), 0.0, 0.0);
    auto midId = scheduler.add(std::move(refMid), 1.0, 0.5);
    auto tailId = scheduler.add(std::move(refTail), 1.0, 0.0);
    REQUIRE(headId != 0);
    REQUIRE(midId != 0);
    REQUIRE(tailId != 0);

    scheduler.advance(0.0, L);
    REQUIRE_FALSE(scheduler.isScheduled(headId));
    REQUIRE(scheduler.isScheduled(midId));
    REQUIRE(scheduler.isScheduled(tailId));

    scheduler.cancel(midId);
    REQUIRE_FALSE(scheduler.isScheduled(midId));
    REQUIRE(scheduler.isScheduled(tailId));

    scheduler.advance(1.0, L);
    REQUIRE_FALSE(scheduler.isScheduled(tailId));

    lua_getglobal(L, "headHit");
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);
    lua_getglobal(L, "midHit");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
    lua_getglobal(L, "tailHit");
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);
}

TEST_CASE("TaskScheduler m_index stays valid after deferred compaction") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto refFirst = luauapi_test::makeCallback(L, "_G.deferFirst = (_G.deferFirst or 0) + 1");
    auto refSecond = luauapi_test::makeCallback(L, "_G.deferSecond = (_G.deferSecond or 0) + 1");

    auto& scheduler = luax::TaskScheduler::get();
    auto firstId = scheduler.addDeferred(std::move(refFirst));
    auto secondId = scheduler.addDeferred(std::move(refSecond));
    REQUIRE(firstId != 0);
    REQUIRE(secondId != 0);

    scheduler.advance(0.0, L);
    REQUIRE_FALSE(scheduler.isScheduled(firstId));
    REQUIRE_FALSE(scheduler.isScheduled(secondId));

    auto refThird = luauapi_test::makeCallback(L, "_G.deferThird = (_G.deferThird or 0) + 1");
    auto thirdId = scheduler.addDeferred(std::move(refThird));
    REQUIRE(thirdId != 0);
    REQUIRE(scheduler.isScheduled(thirdId));

    scheduler.cancel(thirdId);
    REQUIRE_FALSE(scheduler.isScheduled(thirdId));
    scheduler.advance(0.0, L);
    REQUIRE(scheduler.activeCount() == 0);

    lua_getglobal(L, "deferFirst");
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);
    lua_getglobal(L, "deferSecond");
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);
    lua_getglobal(L, "deferThird");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

TEST_CASE("TaskScheduler allows add after cancel without compaction") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto& scheduler = luax::TaskScheduler::get();
    std::vector<std::uint64_t> ids;
    ids.reserve(luax::kMaxScheduledTasks);

    for (std::size_t i = 0; i < luax::kMaxScheduledTasks; ++i) {
        auto ref = luauapi_test::makeCallback(L, "return");
        auto id = scheduler.add(std::move(ref), 1000.0, 0.0);
        REQUIRE(id != 0);
        ids.push_back(id);
    }
    REQUIRE(scheduler.full());
    REQUIRE(scheduler.activeCount() == luax::kMaxScheduledTasks);

    for (std::uint64_t id : ids) {
        scheduler.cancel(id);
    }
    REQUIRE(scheduler.activeCount() == 0);
    REQUIRE_FALSE(scheduler.full());

    auto ref = luauapi_test::makeCallback(L, "return");
    auto newId = scheduler.add(std::move(ref), 0.0, 0.0);
    REQUIRE(newId != 0);
    REQUIRE(scheduler.activeCount() == 1);
}

TEST_CASE(
    "TaskScheduler cancel still resolves tasks after mixed timed and "
    "deferred compaction"
) {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto refDefer = luauapi_test::makeCallback(L, "_G.mixDefer = (_G.mixDefer or 0) + 1");
    auto refTimed = luauapi_test::makeCallback(L, "_G.mixTimed = (_G.mixTimed or 0) + 1");

    auto& scheduler = luax::TaskScheduler::get();
    auto deferId = scheduler.addDeferred(std::move(refDefer));
    auto timedId = scheduler.add(std::move(refTimed), 1.0, 0.0);
    REQUIRE(deferId != 0);
    REQUIRE(timedId != 0);

    scheduler.advance(0.0, L);
    REQUIRE_FALSE(scheduler.isScheduled(deferId));
    REQUIRE(scheduler.isScheduled(timedId));

    scheduler.cancel(timedId);
    REQUIRE_FALSE(scheduler.isScheduled(timedId));
    scheduler.advance(1.0, L);
    REQUIRE(scheduler.activeCount() == 0);

    lua_getglobal(L, "mixDefer");
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);
    lua_getglobal(L, "mixTimed");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}
