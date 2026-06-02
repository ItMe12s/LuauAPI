#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/LuaDelegate.hpp"
#include "lua/bindings/framework/LuaRef.hpp"
#include "lua/bindings/framework/LuaTrampolineRegistry.hpp"
#include "lua/bindings/framework/Usertype.hpp"
#include "lua/bindings/task/TaskScheduler.hpp"
#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"
#include "lua_test_helpers.hpp"

#include <RuntimeTypes.hpp>

#include <catch2/catch_test_macros.hpp>
#include <lua.h>

#include <thread>

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
}

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
    REQUIRE_FALSE(luax::applyAllBindings(L).has_value());

    luauapi_test::loadFunction(L, "function() end");
    lua_getglobal(L, "task");
    lua_getfield(L, -1, "spawn");
    lua_pushvalue(L, -3);

    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::NotReady);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    lua_pop(L, 1);
}
