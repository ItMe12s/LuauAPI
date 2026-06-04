#include "lua/bindings/framework/OpaqueHandle.hpp"
#include "lua/bindings/framework/ReadOnlyVectorView.hpp"
#include "lua/bindings/framework/Usertype.hpp"
#include "lua/runtime/Runtime.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <thread>

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

    void indexReadOnlyOpaqueVectorView(lua_State* L, int viewIndex, int elementIndex) {
        viewIndex = lua_absindex(L, viewIndex);
        lua_getmetatable(L, viewIndex);
        lua_getfield(L, -1, "__index");
        lua_pushvalue(L, viewIndex);
        lua_pushinteger(L, elementIndex);
        lua_call(L, 2, 1);
        lua_remove(L, -2);
    }
} // namespace

TEST_CASE("OpaqueHandle push and check round-trip") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);
    REQUIRE(luax::registerOpaqueHandle(L).isOk());

    int payload = 42;
    luax::pushOpaqueHandle(L, &payload);
    REQUIRE(lua_type(L, -1) == LUA_TUSERDATA);
    REQUIRE(lua_userdatatag(L, -1) == luax::detail::opaqueHandleTag());

    auto* out = luax::checkOpaqueHandle<int>(L, -1, "test");
    REQUIRE(out == &payload);
    lua_pop(L, 1);
}

TEST_CASE("OpaqueHandle rejects light userdata") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(luax::registerOpaqueHandle(L).isOk());

    int payload = 7;
    lua_pushlightuserdata(L, &payload);
    REQUIRE_THROWS_AS(luax::checkOpaqueHandle<int>(L, -1, "test"), std::exception);
}

TEST_CASE("ReadOnlyOpaqueVectorView indexes tagged opaque handles") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(luax::registerOpaqueHandle(L).isOk());
    REQUIRE(luax::Usertype<TestObj>::registerType(L, "TestObj").isOk());

    auto* owner = new TestObj();
    int first = 1;
    int second = 2;
    gd::vector<int*> members;
    members.push_back(&first);
    members.push_back(&second);

    luax::pushReadOnlyOpaqueVectorView<int>(L, members, owner);
    REQUIRE(lua_isuserdata(L, -1));

    indexReadOnlyOpaqueVectorView(L, -1, 1);
    REQUIRE(lua_userdatatag(L, -1) == luax::detail::opaqueHandleTag());
    REQUIRE(luax::checkOpaqueHandle<int>(L, -1, "elem") == &first);
    lua_pop(L, 1);

    indexReadOnlyOpaqueVectorView(L, -1, 2);
    REQUIRE(luax::checkOpaqueHandle<int>(L, -1, "elem") == &second);
    lua_pop(L, 2);

    owner->release();
}

TEST_CASE("checkOpaqueVectorView accepts table of opaque handles") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(luax::registerOpaqueHandle(L).isOk());

    int first = 11;
    int second = 22;
    lua_createtable(L, 2, 0);
    luax::pushOpaqueHandle(L, &first);
    lua_rawseti(L, -2, 1);
    luax::pushOpaqueHandle(L, &second);
    lua_rawseti(L, -2, 2);

    auto copied = luax::checkOpaqueVectorView<int>(L, -1, "test");
    REQUIRE(copied.size() == 2);
    REQUIRE(copied[0] == &first);
    REQUIRE(copied[1] == &second);
    lua_pop(L, 1);
}
