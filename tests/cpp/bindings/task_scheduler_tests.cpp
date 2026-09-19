#include "bindings/task/TaskScheduler.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "framework/usertype/Usertype.hpp"
#include "host/lua_test_helpers.hpp"

#include <RuntimeTypes.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>
#include <string_view>
#include <thread>
#include <vector>

namespace luax {
    geode::Result<void> registerTask(lua_State* L);
} // namespace luax

namespace {
    using RuntimeGuard = luauapi_test::TaskSchedulerRuntimeGuard;
    using luauapi_test::globalInteger;
    using luauapi_test::globalIsNil;

    void registerTaskBinding(lua_State* L) {
        luax::registerBinding({"task_lib", &luax::registerTask, 10});
        REQUIRE(luax::applyAllBindings(L) == std::nullopt);
    }

    void registerNodeType(lua_State* L) {
        REQUIRE(luax::Usertype<cocos2d::CCNode>::registerType(L, "CCNode").isOk());
    }

    void pushNodeGlobal(lua_State* L, cocos2d::CCNode* node, char const* name) {
        luax::Usertype<cocos2d::CCNode>::pushBorrowed(L, node);
        lua_setglobal(L, name);
    }

    void clearNodeGlobal(lua_State* L, char const* name, cocos2d::CCNode* node) {
        lua_pushnil(L);
        lua_setglobal(L, name);
        node->release();
        lua_gc(L, LUA_GCCOLLECT, 0);
    }
} // namespace

TEST_CASE("TaskScheduler fires one-shot tasks after delay") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    lua_pushinteger(L, 0);
    lua_setglobal(L, "hits");

    auto ref = luauapi_test::makeCallback(L, R"(
        local hits = _G.hits or 0
        _G.hits = hits + 1
    )");

    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.add(std::move(ref), 0.1, 0.0);
    REQUIRE(id != 0);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.advance(0.05);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.advance(0.1);
    REQUIRE(scheduler.activeCount() == 0);
    REQUIRE(globalInteger(L, "hits") == 1);
}

TEST_CASE("TaskScheduler repeats interval tasks until cancelled") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "_G.intervalHits = (_G.intervalHits or 0) + 1");

    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.add(std::move(ref), 0.0, 0.5);
    REQUIRE(id != 0);

    scheduler.advance(0.5);
    scheduler.advance(0.5);
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.cancel(id);
    scheduler.advance(0.5);
    REQUIRE(scheduler.activeCount() == 0);
    REQUIRE(globalInteger(L, "intervalHits") == 2);
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
    scheduler.advance(0.0);
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

    scheduler.advance(2.0);
    REQUIRE(scheduler.activeCount() == 1);
    REQUIRE(globalInteger(L, "bigDtHits") == 1);
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

    scheduler.advance(0.0);
    REQUIRE(scheduler.activeCount() == 0);
    REQUIRE(globalInteger(L, "deferHits") == 1);
}

TEST_CASE("TaskScheduler cancels tasks that error") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "error('task failed')");

    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.add(std::move(ref), 0.0, 0.0);
    REQUIRE(id != 0);

    scheduler.advance(0.0);
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

    scheduler.advance(0.0);
    REQUIRE_FALSE(scheduler.isScheduled(headId));
    REQUIRE(scheduler.isScheduled(midId));
    REQUIRE(scheduler.isScheduled(tailId));

    scheduler.cancel(midId);
    REQUIRE_FALSE(scheduler.isScheduled(midId));
    REQUIRE(scheduler.isScheduled(tailId));

    scheduler.advance(1.0);
    REQUIRE_FALSE(scheduler.isScheduled(tailId));

    REQUIRE(globalInteger(L, "headHit") == 1);
    REQUIRE(globalIsNil(L, "midHit"));
    REQUIRE(globalInteger(L, "tailHit") == 1);
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

    scheduler.advance(0.0);
    REQUIRE_FALSE(scheduler.isScheduled(firstId));
    REQUIRE_FALSE(scheduler.isScheduled(secondId));

    auto refThird = luauapi_test::makeCallback(L, "_G.deferThird = (_G.deferThird or 0) + 1");
    auto thirdId = scheduler.addDeferred(std::move(refThird));
    REQUIRE(thirdId != 0);
    REQUIRE(scheduler.isScheduled(thirdId));

    scheduler.cancel(thirdId);
    REQUIRE_FALSE(scheduler.isScheduled(thirdId));
    scheduler.advance(0.0);
    REQUIRE(scheduler.activeCount() == 0);

    REQUIRE(globalInteger(L, "deferFirst") == 1);
    REQUIRE(globalInteger(L, "deferSecond") == 1);
    REQUIRE(globalIsNil(L, "deferThird"));
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

    scheduler.advance(0.0);
    REQUIRE_FALSE(scheduler.isScheduled(deferId));
    REQUIRE(scheduler.isScheduled(timedId));

    scheduler.cancel(timedId);
    REQUIRE_FALSE(scheduler.isScheduled(timedId));
    scheduler.advance(1.0);
    REQUIRE(scheduler.activeCount() == 0);

    REQUIRE(globalInteger(L, "mixDefer") == 1);
    REQUIRE(globalIsNil(L, "mixTimed"));
}

TEST_CASE("task.wait works inside task.delay callbacks") {
    luauapi_test::HandleGcRuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerTaskBinding(L);

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            task.delay(0, function()
                local elapsed = task.wait(0.1)
                _G.delayWaitElapsed = elapsed
                _G.delayWaitDone = true
            end)
        )"
        )
    );

    auto& scheduler = luax::TaskScheduler::get();
    scheduler.advance(0.0);
    REQUIRE(scheduler.activeCount() == 1);
    REQUIRE(globalIsNil(L, "delayWaitDone"));

    scheduler.advance(0.15);
    REQUIRE(scheduler.activeCount() == 0);

    REQUIRE(luauapi_test::globalBool(L, "delayWaitDone"));
    REQUIRE(luauapi_test::globalNumber(L, "delayWaitElapsed") == Catch::Approx(0.15));
}

TEST_CASE("task.wait errors when called outside a yieldable thread") {
    luauapi_test::HandleGcRuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerTaskBinding(L);

    luauapi_test::loadFunction(L, "task.wait(0)");
    REQUIRE(lua_pcall(L, 0, 0, 0) != 0);
    char const* err = lua_tostring(L, -1);
    REQUIRE(err != nullptr);
    REQUIRE(
        std::string_view(err).contains("task.wait must be called from a coroutine or task callback")
    );
    lua_pop(L, 1);
}

TEST_CASE("TaskScheduler everyNode fires while the node is running") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref = luauapi_test::makeCallback(L, "_G.everyNodeHits = (_G.everyNodeHits or 0) + 1");

    auto* node = new cocos2d::CCNode();
    node->retain();
    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.addForNode(std::move(ref), node, 0.0, 0.5);
    REQUIRE(id != 0);
    REQUIRE(scheduler.activeCount() == 1);
    node->release();

    scheduler.advance(0.5);
    scheduler.advance(0.5);
    REQUIRE(scheduler.activeCount() == 1);
    REQUIRE(globalInteger(L, "everyNodeHits") == 2);

    node->release();
}

TEST_CASE("TaskScheduler everyNode cancels when the node stops running") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref =
        luauapi_test::makeCallback(L, "_G.everyNodeStopsHits = (_G.everyNodeStopsHits or 0) + 1");

    auto* node = new cocos2d::CCNode();
    node->retain();
    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.addForNode(std::move(ref), node, 0.0, 0.5);
    REQUIRE(id != 0);
    node->release();

    scheduler.advance(0.5);
    REQUIRE(scheduler.isScheduled(id));
    REQUIRE(globalInteger(L, "everyNodeStopsHits") == 1);

    node->setRunningForTests(false);
    scheduler.advance(0.5);
    REQUIRE_FALSE(scheduler.isScheduled(id));

    node->release();
}

TEST_CASE("TaskScheduler everyNode cancels when the node is freed") {
    luauapi_test::WeakRefPoolSimGuard poolGuard;

    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto ref =
        luauapi_test::makeCallback(L, "_G.everyNodeFreedHits = (_G.everyNodeFreedHits or 0) + 1");

    auto* node = new cocos2d::CCNode();
    auto& scheduler = luax::TaskScheduler::get();
    auto id = scheduler.addForNode(std::move(ref), node, 0.0, 0.5);
    REQUIRE(id != 0);
    node->release();

    scheduler.advance(0.5);
    REQUIRE_FALSE(scheduler.isScheduled(id));
    REQUIRE(globalIsNil(L, "everyNodeFreedHits"));
}

TEST_CASE("task.everyNode fires from Lua and cancels on handle cancel") {
    luauapi_test::HandleGcRuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerTaskBinding(L);
    registerNodeType(L);

    auto* node = new cocos2d::CCNode();
    node->retain();
    pushNodeGlobal(L, node, "everyNodeTarget");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.everyNodeHits = 0
            _G.everyNodeHandle = task.everyNode(everyNodeTarget, 0.1, function()
                _G.everyNodeHits = _G.everyNodeHits + 1
            end)
        )"
        )
    );

    auto& scheduler = luax::TaskScheduler::get();
    scheduler.advance(0.1);
    REQUIRE(scheduler.activeCount() == 1);

    REQUIRE(luauapi_test::runScriptVoid(L, "_G.everyNodeHandle:cancel()"));
    scheduler.advance(0.1);

    REQUIRE(globalInteger(L, "everyNodeHits") == 1);

    clearNodeGlobal(L, "everyNodeTarget", node);
}

TEST_CASE("task.everyNode rejects a non-node argument") {
    luauapi_test::HandleGcRuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerTaskBinding(L);

    luauapi_test::loadFunction(L, "task.everyNode(123, 0.1, function() end)");
    REQUIRE(lua_pcall(L, 0, 0, 0) != 0);
    char const* err = lua_tostring(L, -1);
    REQUIRE(err != nullptr);
    REQUIRE(std::string_view(err).contains("task.everyNode: expected a CCNode at arg 1"));
    lua_pop(L, 1);
}

TEST_CASE("task.everyNode cancels on first tick when the node is not running") {
    luauapi_test::HandleGcRuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerTaskBinding(L);
    registerNodeType(L);

    auto* node = new cocos2d::CCNode();
    node->setRunningForTests(false);
    pushNodeGlobal(L, node, "everyNodeTarget");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.everyNodeHits = 0
            _G.everyNodeHandle = task.everyNode(everyNodeTarget, 0.1, function()
                _G.everyNodeHits = _G.everyNodeHits + 1
            end)
        )"
        )
    );

    auto& scheduler = luax::TaskScheduler::get();
    REQUIRE(scheduler.activeCount() == 1);

    scheduler.advance(0.1);

    REQUIRE(globalInteger(L, "everyNodeHits") == 0);
    REQUIRE(scheduler.activeCount() == 0);

    clearNodeGlobal(L, "everyNodeTarget", node);
}

TEST_CASE("task.everyNode rejects a non-positive interval") {
    luauapi_test::HandleGcRuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerTaskBinding(L);
    registerNodeType(L);

    auto* node = new cocos2d::CCNode();
    pushNodeGlobal(L, node, "everyNodeTarget");

    luauapi_test::loadFunction(L, "task.everyNode(everyNodeTarget, 0, function() end)");
    REQUIRE(lua_pcall(L, 0, 0, 0) != 0);
    char const* err = lua_tostring(L, -1);
    REQUIRE(err != nullptr);
    REQUIRE(std::string_view(err).contains("task.everyNode: interval must be > 0"));
    lua_pop(L, 1);

    clearNodeGlobal(L, "everyNodeTarget", node);
}
