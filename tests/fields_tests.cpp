#include "core/Runtime.hpp"
#include "framework/usertype/Fields.hpp"
#include "host/lua_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <lua.h>
#include <optional>
#include <string>

namespace {
    struct TestNode : cocos2d::CCNode {};

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

    TestNode* allocateAtAddress(std::uintptr_t target, int maxAttempts = 256) {
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            auto* node = new TestNode();
            auto address = reinterpret_cast<std::uintptr_t>(node);
            if (address == target) {
                return node;
            }
            node->release();
        }
        return nullptr;
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

    auto* first = new TestNode();
    auto const reusedAddress = reinterpret_cast<std::uintptr_t>(first);

    luax::Fields::push(L, first);
    REQUIRE(lua_istable(L, -1));
    setTableToken(L, "stale");
    lua_pop(L, 1);

    first->release();

    auto* second = allocateAtAddress(reusedAddress);
    REQUIRE(second != nullptr);
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

    second->release();
}
