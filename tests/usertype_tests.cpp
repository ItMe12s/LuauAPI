#include "lua/bindings/framework/Fields.hpp"
#include "lua/bindings/framework/Usertype.hpp"
#include "lua/runtime/Runtime.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>

#include <string>
#include <typeindex>

namespace {
    struct TestNode : cocos2d::CCNode {};

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

    int testMethod(lua_State* L) {
        lua_pushliteral(L, "called");
        return 1;
    }
}

TEST_CASE("UsertypeRegistry assigns unique tags") {
    RuntimeGuard guard;
    auto& reg = luax::detail::UsertypeRegistry::get();

    auto first = reg.tagFor(std::type_index(typeid(TestNode)));
    auto second = reg.tagFor(std::type_index(typeid(cocos2d::CCNode)));
    REQUIRE(first != 0);
    REQUIRE(second != 0);
    REQUIRE(first != second);
    REQUIRE(reg.tagFor(std::type_index(typeid(TestNode))) == first);
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
