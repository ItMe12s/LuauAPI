#include "lua/bindings/framework/ReadOnlyVectorView.hpp"
#include "lua/bindings/framework/Usertype.hpp"
#include "lua/runtime/Runtime.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>

#include <thread>
#include <vector>

namespace {
    struct TestObj : cocos2d::CCObject {};

    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::Runtime::resetForTests();
        }
    };
}

TEST_CASE("ReadOnlyVectorView snapshots borrowed vector storage") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(luax::Usertype<TestObj>::registerType(L, "TestObj").isOk());

    auto* owner = new TestObj();
    gd::vector<cocos2d::CCObject*> members;
    auto* child = new TestObj();
    members.push_back(child);

    luax::pushReadOnlyVectorView<cocos2d::CCObject>(L, members, owner);
    REQUIRE(lua_isuserdata(L, -1));

    members.clear();
    lua_pushnumber(L, 1);
    REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
    REQUIRE(lua_isuserdata(L, -1));

    child->release();
    owner->release();
    lua_pop(L, 1);
}

TEST_CASE("ReadOnlyVectorView errors when owner is gone") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestObj>::registerType(L, "TestObj").isOk());

    auto* owner = new TestObj();
    gd::vector<cocos2d::CCObject*> members;
    luax::pushReadOnlyVectorView<cocos2d::CCObject>(L, members, owner);
    owner->release();

    lua_pushnumber(L, 1);
    REQUIRE(lua_pcall(L, 1, 1, 0) != 0);
    lua_pop(L, 1);
}
