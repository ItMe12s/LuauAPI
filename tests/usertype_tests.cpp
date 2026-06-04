#include "lua/bindings/framework/Fields.hpp"
#include "lua/bindings/framework/Usertype.hpp"
#include "lua/runtime/Runtime.hpp"

#include <RuntimeTypes.hpp>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <typeindex>

namespace {
    struct TestNode : cocos2d::CCNode {};

    struct OverflowNode : cocos2d::CCNode {};

    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::Fields::clear();
            luax::Runtime::resetForTests();
        }
    };

    int getTestField(lua_State* L) {
        lua_pushliteral(L, "value");
        return 1;
    }

    int setTestField(lua_State* L) {
        (void)L;
        return 0;
    }

    int erroringGetField(lua_State* L) {
        luaL_error(L, "field getter boom");
        return 0;
    }

    int erroringSetField(lua_State* L) {
        luaL_error(L, "field setter boom");
        return 0;
    }

    int testMethod(lua_State* L) {
        lua_pushliteral(L, "called");
        return 1;
    }
} // namespace

TEST_CASE("UsertypeRegistry tagFor returns zero when tag limit is exceeded") {
    RuntimeGuard guard;
    auto& reg = luax::detail::UsertypeRegistry::get();
    reg.setNextTagForTests(LUA_UTAG_LIMIT);

    REQUIRE(reg.tagFor(std::type_index(typeid(OverflowNode))) == 0);
}

TEST_CASE("UsertypeRegistry returns error when tag limit is exceeded") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto& reg = luax::detail::UsertypeRegistry::get();
    reg.setNextTagForTests(LUA_UTAG_LIMIT);

    auto result = luax::Usertype<OverflowNode>::registerType(L, "OverflowNode");
    REQUIRE(result.isErr());
}

TEST_CASE("Usertype cannot push userdata when tag registration fails") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto& reg = luax::detail::UsertypeRegistry::get();
    reg.setNextTagForTests(LUA_UTAG_LIMIT);

    REQUIRE(luax::Usertype<OverflowNode>::registerType(L, "OverflowNode").isErr());
    REQUIRE(luax::Usertype<OverflowNode>::tag() == 0);

    auto* node = new OverflowNode();
    luax::Usertype<OverflowNode>::pushBorrowed(L, node);
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
    node->release();
}

TEST_CASE("UsertypeRegistry assigns unique tags") {
    RuntimeGuard guard;
    auto& reg = luax::detail::UsertypeRegistry::get();

    auto firstResult = reg.ensureInfo(std::type_index(typeid(TestNode)));
    auto secondResult = reg.ensureInfo(std::type_index(typeid(cocos2d::CCNode)));
    REQUIRE(firstResult.isOk());
    REQUIRE(secondResult.isOk());
    auto first = firstResult.unwrap()->tag;
    auto second = secondResult.unwrap()->tag;
    REQUIRE(first != 0);
    REQUIRE(second != 0);
    REQUIRE(first != second);
    REQUIRE(reg.tagFor(std::type_index(typeid(TestNode))) == first);
}

TEST_CASE("Usertype tag returns zero before registration") {
    RuntimeGuard guard;
    REQUIRE(luax::Usertype<TestNode>::tag() == 0);
}

TEST_CASE("Usertype registerType rejects invalid base tag") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto result = luax::Usertype<TestNode>::registerType(L, "TestNode", {0});
    REQUIRE(result.isErr());
}

TEST_CASE("Usertype pushBorrowed returns nil when type is not registered") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
    node->release();
}

TEST_CASE("Usertype metatable dispatches methods and fields") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    auto result = luax::Usertype<TestNode>::registerType(L, "TestNode");
    REQUIRE(result.isOk());

    luax::Usertype<TestNode>::method(L, "ping", &testMethod);
    luax::Usertype<TestNode>::field(L, "answer", &getTestField, &setTestField);

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    REQUIRE(lua_isuserdata(L, -1));

    lua_getfield(L, -1, "ping");
    REQUIRE(lua_isfunction(L, -1));
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
    REQUIRE(std::string_view(lua_tostring(L, -1)) == "called");
    lua_pop(L, 1);

    lua_getfield(L, -1, "answer");
    REQUIRE(lua_isstring(L, -1));
    REQUIRE(std::string_view(lua_tostring(L, -1)) == "value");
    lua_pop(L, 2);

    node->release();
}

TEST_CASE(
    "Usertype __index returns nil for unknown field without "
    "materializing m_fields"
) {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);

    lua_getfield(L, -1, "missingField");
    REQUIRE(lua_isnil(L, -1));
    REQUIRE_FALSE(luax::Fields::tryPush(L, node));

    lua_pop(L, 2);
    node->release();
}

TEST_CASE("Usertype rejects writes to read-only m_fields") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());

    auto* node = new TestNode();
    lua_pushcfunction(
        L,
        [](lua_State* S) -> int {
            lua_setfield(S, 1, "m_fields");
            return 0;
        },
        "assignMFields"
    );
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    lua_newtable(L);
    REQUIRE(lua_pcall(L, 2, 0, 0) != 0);
    lua_pop(L, 1);

    node->release();
}

TEST_CASE("Usertype field getter uses traceback when runtime is not ready") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());
    luax::Usertype<TestNode>::field(L, "boomField", &erroringGetField, &setTestField);

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);

    runtime->clearLastError();
    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::NotReady);

    int topBefore = lua_gettop(L);
    lua_getfield(L, -1, "boomField");
    REQUIRE(lua_gettop(L) == topBefore + 1);
    REQUIRE(lua_isnil(L, -1));

    auto const& err = runtime->lastError();
    REQUIRE(err.find("field getter boom") != std::string::npos);
    REQUIRE(err.find("stack:") != std::string::npos);

    lua_pop(L, 2);
    node->release();
}

TEST_CASE("Usertype field setter uses traceback when runtime is not ready") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());
    luax::Usertype<TestNode>::field(L, "boomField", &getTestField, &erroringSetField);

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);

    runtime->clearLastError();
    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::NotReady);

    lua_pushcfunction(
        L,
        [](lua_State* S) -> int {
            lua_pushliteral(S, "value");
            lua_setfield(S, 1, "boomField");
            return 0;
        },
        "assignBoomField"
    );
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    lua_pop(L, 1);

    auto const& err = runtime->lastError();
    REQUIRE(err.find("field setter boom") != std::string::npos);
    REQUIRE(err.find("stack:") != std::string::npos);

    lua_pop(L, 1);
    node->release();
}
