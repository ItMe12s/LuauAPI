#include "core/Runtime.hpp"
#include "framework/usertype/Fields.hpp"
#include "host/lua_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstdlib>
#include <lua.h>
#include <new>
#include <optional>
#include <string>

namespace {
    struct TestNode : cocos2d::CCNode {};

    struct PlainObject : cocos2d::CCObject {};

    using RuntimeGuard = luauapi_test::FieldsRuntimeGuard;

    void setTableToken(lua_State* L, char const* token) {
        lua_pushstring(L, token);
        lua_setfield(L, -2, "token");
    }

    std::optional<std::string> getTableToken(lua_State* L) {
        lua_getfield(L, -1, "token");
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        std::string value = lua_tostring(L, -1);
        lua_pop(L, 1);
        return value;
    }

} // namespace

TEST_CASE("Fields tryPush returns false without materializing a table") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto* node = new TestNode();
    REQUIRE_FALSE(luax::Fields::tryPush(L, node));
    REQUIRE(lua_gettop(L) == 0);

    luax::Fields::push(L, node);
    REQUIRE(lua_istable(L, -1));
    lua_pop(L, 1);

    node->release();
}

TEST_CASE("Fields evictIfFinalRelease is no-op without a field entry") {
    RuntimeGuard guard;
    auto* node = new TestNode();

    REQUIRE_NOTHROW(luax::Fields::evictIfFinalRelease(node));

    node->release();
}

TEST_CASE("Fields evictIfFinalRelease is no-op for non-node CCObject") {
    RuntimeGuard guard;
    auto* object = new PlainObject();

    REQUIRE_NOTHROW(luax::Fields::evictIfFinalRelease(object));

    object->release();
}

TEST_CASE("Fields evictIfFinalRelease removes entry on final release") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto* node = new TestNode();
    luax::Fields::push(L, node);
    setTableToken(L, "first");
    lua_pop(L, 1);

    luax::Fields::evictIfFinalRelease(node);
    luax::Fields::push(L, node);
    REQUIRE(getTableToken(L) == std::nullopt);
    lua_pop(L, 1);

    node->retain();
    luax::Fields::push(L, node);
    setTableToken(L, "retained");
    lua_pop(L, 1);

    luax::Fields::evictIfFinalRelease(node);
    luax::Fields::push(L, node);
    REQUIRE(getTableToken(L) == "retained");
    lua_pop(L, 1);

    node->release();
    luax::Fields::evictIfFinalRelease(node);
    luax::Fields::push(L, node);
    REQUIRE(getTableToken(L) == std::nullopt);
    lua_pop(L, 1);

    node->release();
}

TEST_CASE("Fields push drops stale map entry when node address is reused") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    void* mem = std::malloc(sizeof(TestNode));
    REQUIRE(mem != nullptr);

    auto* first = new (mem) TestNode();
    auto const reusedAddress = reinterpret_cast<std::uintptr_t>(mem);

    luax::Fields::push(L, first);
    REQUIRE(lua_istable(L, -1));
    setTableToken(L, "stale");
    lua_pop(L, 1);

    first->~TestNode();

    auto* second = new (mem) TestNode();
    REQUIRE(reinterpret_cast<std::uintptr_t>(second) == reusedAddress);

    luax::Fields::push(L, second);
    REQUIRE(lua_istable(L, -1));
    REQUIRE(getTableToken(L) == std::nullopt);

    setTableToken(L, "fresh");
    lua_pop(L, 1);

    luax::Fields::push(L, second);
    REQUIRE(lua_istable(L, -1));
    REQUIRE(getTableToken(L) == "fresh");
    lua_pop(L, 1);

    second->~TestNode();
    std::free(mem);
}
