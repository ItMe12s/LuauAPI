#include "framework/view/ReadOnlyVectorView.hpp"
#include "framework/usertype/Usertype.hpp"
#include "core/Runtime.hpp"

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

    void indexReadOnlyVectorView(lua_State* L, int viewIndex, int elementIndex) {
        viewIndex = lua_absindex(L, viewIndex);
        lua_getmetatable(L, viewIndex);
        lua_getfield(L, -1, "__index");
        lua_pushvalue(L, viewIndex);
        lua_pushinteger(L, elementIndex);
        lua_call(L, 2, 1);
        lua_remove(L, -2);
    }

    int pcallIndexReadOnlyVectorView(lua_State* L, int viewIndex, int elementIndex) {
        viewIndex = lua_absindex(L, viewIndex);
        lua_getmetatable(L, viewIndex);
        lua_getfield(L, -1, "__index");
        lua_pushvalue(L, viewIndex);
        lua_pushinteger(L, elementIndex);
        lua_remove(L, -4);
        return lua_pcall(L, 2, 1, 0);
    }
} // namespace

TEST_CASE("ReadOnlyVectorView tracks borrowed vector storage") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(luax::Usertype<TestObj>::registerType(L, "TestObj").isOk());

    auto* owner = new TestObj();
    gd::vector<TestObj*> members;
    auto* child = new TestObj();
    members.push_back(child);

    luax::pushReadOnlyVectorView<TestObj>(L, members, owner);
    REQUIRE(lua_isuserdata(L, -1));

    members.clear();
    indexReadOnlyVectorView(L, -1, 1);
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);

    members.push_back(child);
    indexReadOnlyVectorView(L, -1, 1);
    REQUIRE(lua_isuserdata(L, -1));

    child->release();
    owner->release();
    lua_pop(L, 1);
}

TEST_CASE("ReadOnlyVectorView rejects access after owner release") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestObj>::registerType(L, "TestObj").isOk());

    auto* owner = new TestObj();
    auto* child = new TestObj();
    gd::vector<TestObj*> members;
    members.push_back(child);
    luax::pushReadOnlyVectorView<TestObj>(L, members, owner);
    owner->release();

    REQUIRE(pcallIndexReadOnlyVectorView(L, -1, 1) != 0);
    REQUIRE(lua_isstring(L, -1));

    child->release();
    lua_pop(L, 1);
}
