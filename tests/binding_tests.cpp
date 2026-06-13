#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/TaggedMetatable.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>
#include <string>

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::resetBindingsForTests();
        }

        ~RuntimeGuard() {
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };

    struct TestPayload {
        int value = 0;
    };

    bool g_metatableGcCalled = false;
    bool g_dtorCalled = false;

    int untaggedMetatableGc(lua_State* L) {
        g_metatableGcCalled = true;
        (void)L;
        return 0;
    }

    void taggedDtor(lua_State*, void* ud) {
        g_dtorCalled = true;
        (void)ud;
    }

    int getValue(lua_State* L) {
        auto* payload = static_cast<TestPayload*>(lua_touserdata(L, 1));
        lua_pushinteger(L, payload->value);
        return 1;
    }

    struct LuaStateGuard {
        lua_State* L = luaL_newstate();

        ~LuaStateGuard() {
            if (L) {
                lua_close(L);
            }
        }
    };

    void collectGarbage(lua_State* L) {
        lua_gc(L, LUA_GCSTOP, 0);
        lua_gc(L, LUA_GCCOLLECT, 0);
        lua_gc(L, LUA_GCRESTART, 0);
    }

    geode::Result<void> registerOk(lua_State* L) {
        lua_pushinteger(L, 1);
        lua_setglobal(L, "binding_ok");
        return geode::Ok();
    }

    geode::Result<void> registerFail(lua_State* L) {
        (void)L;
        return geode::Err("binding failed");
    }
} // namespace

TEST_CASE("applyAllBindings sorts registrars by priority") {
    RuntimeGuard guard;
    luax::registerBinding({"high", &registerOk, 1});
    luax::registerBinding({"low", &registerFail, 100});
    luax::registerBinding({"mid", &registerOk, 50});

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    REQUIRE(runtime->ready());

    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    auto error = luax::applyAllBindings(L);
    REQUIRE(error.has_value());
    REQUIRE(error->find("low") != std::string::npos);

    lua_getglobal(L, "binding_ok");
    REQUIRE(lua_isnumber(L, -1));
    REQUIRE(lua_tointeger(L, -1) == 1);
    lua_pop(L, 1);
}

TEST_CASE("applyAllBindings preserves stable order for equal priority") {
    RuntimeGuard guard;
    luax::registerBinding({"first", &registerOk, 10});
    luax::registerBinding({"second", &registerOk, 10});

    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(luax::applyAllBindings(L) == std::nullopt);
}

TEST_CASE("registerTaggedMetatable tagged round-trip") {
    g_dtorCalled = false;

    LuaStateGuard guard;
    auto* L = guard.L;
    REQUIRE(L != nullptr);

    constexpr char const* kMeta = "luax.test.TaggedPayload";
    constexpr char const* kTypeName = "TestPayload";
    constexpr int kTag = 99;

    luaL_Reg methods[] = {
        {"getValue", getValue},
        {nullptr, nullptr},
    };

    luax::registerTaggedMetatable(L, kMeta, kTag, methods, std::nullopt, &taggedDtor, kTypeName);
    luax::registerTaggedMetatable(L, kMeta, kTag, methods, std::nullopt, &taggedDtor, kTypeName);

    auto* payload =
        static_cast<TestPayload*>(lua_newuserdatataggedwithmetatable(L, sizeof(TestPayload), kTag));
    payload->value = 7;

    REQUIRE(lua_userdatatag(L, -1) == kTag);

    lua_getmetatable(L, -1);
    REQUIRE(lua_istable(L, -1));
    lua_getfield(L, -1, "__type");
    REQUIRE(lua_isstring(L, -1));
    REQUIRE(std::string(lua_tostring(L, -1)) == kTypeName);
    lua_pop(L, 2);

    lua_getfield(L, -1, "getValue");
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
    REQUIRE(lua_tointeger(L, -1) == 7);
    lua_pop(L, 2);

    lua_pop(L, 1);
    collectGarbage(L);
    REQUIRE(g_dtorCalled);
}

TEST_CASE("registerTaggedMetatable untagged round-trip with metatable gc") {
    g_metatableGcCalled = false;

    LuaStateGuard guard;
    auto* L = guard.L;
    REQUIRE(L != nullptr);

    constexpr char const* kMeta = "luax.test.UntaggedPayload";

    luaL_Reg methods[] = {
        {"getValue", getValue},
        {nullptr, nullptr},
    };

    luax::registerTaggedMetatable(L, kMeta, std::nullopt, methods, &untaggedMetatableGc);

    auto* payload = static_cast<TestPayload*>(lua_newuserdata(L, sizeof(TestPayload)));
    payload->value = 3;
    luaL_getmetatable(L, kMeta);
    lua_setmetatable(L, -2);

    lua_getmetatable(L, -1);
    lua_getfield(L, -1, "__metatable");
    REQUIRE(lua_isstring(L, -1));
    REQUIRE(std::string(lua_tostring(L, -1)) == "locked");
    lua_pop(L, 3);

    lua_pop(L, 1);
    collectGarbage(L);
    REQUIRE(g_metatableGcCalled);
}
