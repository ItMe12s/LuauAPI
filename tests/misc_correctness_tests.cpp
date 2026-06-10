#include "host/lua_test_helpers.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaDelegate.hpp"
#include "framework/usertype/LuaRef.hpp"
#include "framework/callback/LuaTrampolineRegistry.hpp"
#include "framework/usertype/Usertype.hpp"
#include "bindings/task/TaskScheduler.hpp"
#include "core/Runtime.hpp"

#include <RuntimeTypes.hpp>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <thread>

namespace luax {
    geode::Result<void> registerTask(lua_State* L);
}

namespace {
    struct TestNode : cocos2d::CCNode {};

    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::resetBindingsForTests();
        }

        ~RuntimeGuard() {
            luax::TaskScheduler::get().clear();
            luax::clearOrphanTrampolines();
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };
} // namespace

TEST_CASE("Orphan trampoline registry rejects registrations past cap") {
    RuntimeGuard guard;
    luax::clearOrphanTrampolines();

    struct Trampoline : cocos2d::CCObject {};

    for (std::size_t i = 0; i < luax::kMaxCallbackTrampolines; ++i) {
        luax::registerOrphanTrampoline(new Trampoline());
    }

    auto* extra = new Trampoline();
    luax::registerOrphanTrampoline(extra);
    REQUIRE(extra->retainCount() == 1);
    extra->release();
    luax::clearOrphanTrampolines();
}

TEST_CASE("Delegate table lookup prunes invalid refs while shared_ptr is alive") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    void* key = reinterpret_cast<void*>(0xDEF);
    auto table = std::make_shared<luax::LuaRef>();
    lua_newtable(L);
    table->reset(L, -1);
    lua_pop(L, 1);
    luax::LuaDelegateBase::registerInterface(key, table);

    table->reset();
    REQUIRE_FALSE(table->valid());

    REQUIRE(luax::tryPushBoundDelegateTable(L, key));
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);

    REQUIRE(luax::tryPushBoundDelegateTable(L, key));
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

TEST_CASE("Delegate table lookup prunes expired weak entries") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    void* key = reinterpret_cast<void*>(0xABC);
    {
        auto table = std::make_shared<luax::LuaRef>();
        lua_newtable(L);
        table->reset(L, -1);
        lua_pop(L, 1);
        luax::LuaDelegateBase::registerInterface(key, table);
    }

    REQUIRE(luax::tryPushBoundDelegateTable(L, key));
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

TEST_CASE("task.spawn errors when runtime is not ready") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    luax::registerBinding({"task_lib", &luax::registerTask, 10});
    REQUIRE(luax::applyAllBindings(L) == std::nullopt);

    auto callback = luauapi_test::makeCallback(L, "return");
    lua_getglobal(L, "task");
    lua_getfield(L, -1, "spawn");
    if (!callback.push()) {
        FAIL("expected task callback ref to remain valid");
    }

    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::NotReady);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    lua_pop(L, 1);
}
