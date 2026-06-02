#include "lua/runtime/Runtime.hpp"
#include "lua_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>

#include <string>
#include <thread>

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::Runtime::resetForTests();
        }
    };

    void pushReturnValue(lua_State* L, int value) {
        luauapi_test::loadFunction(L, std::string("return ") + std::to_string(value));
    }
}

TEST_CASE("Runtime initializes and shuts down cleanly") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();

    REQUIRE(runtime != nullptr);
    REQUIRE(luax::Runtime::isInitialized());
    REQUIRE(runtime->ready());
    REQUIRE(runtime->status() == imes::luauapi::RuntimeStatus::Ready);
    REQUIRE(runtime->state() != nullptr);
}

TEST_CASE("Runtime shutdown clears initialization state") {
    {
        RuntimeGuard guard;
        REQUIRE(luax::Runtime::getOrCreate() != nullptr);
        REQUIRE(luax::Runtime::isInitialized());
    }

    REQUIRE_FALSE(luax::Runtime::isInitialized());
    REQUIRE(luax::Runtime::getIfInitialized() == nullptr);
}

TEST_CASE("protectedCall restores stack height on success") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    pushReturnValue(L, 99);
    int topBefore = lua_gettop(L);
    REQUIRE(runtime->protectedCall(0, 1, "test"));
    REQUIRE(lua_gettop(L) == topBefore + 1);
    REQUIRE(lua_tointeger(L, -1) == 99);
    lua_pop(L, 1);
    REQUIRE(lua_gettop(L) == topBefore);
}

TEST_CASE("protectedCall restores stack height on failure") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    lua_pushcfunction(
        L,
        [](lua_State* state) -> int {
            luaL_error(state, "boom");
            return 0;
        },
        "fail"
    );
    int topBefore = lua_gettop(L);
    REQUIRE_FALSE(runtime->protectedCall(0, 0, "test"));
    REQUIRE(lua_gettop(L) == topBefore);
    REQUIRE_FALSE(runtime->lastError().empty());
}

TEST_CASE("runScript executes and reports errors") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();

    REQUIRE(runtime->runScript("return 5", "@ok.luau"));
    REQUIRE(runtime->lastError().empty());

    REQUIRE_FALSE(runtime->runScript("return (", "@bad.luau"));
    REQUIRE_FALSE(runtime->lastError().empty());
}

TEST_CASE("Runtime rejects off-main-thread access") {
    RuntimeGuard guard;
    luax::Runtime::getOrCreate();

    std::optional<bool> offThreadReady;
    std::thread worker([&] {
        auto* runtime = luax::Runtime::getIfInitialized();
        offThreadReady = runtime && runtime->ready();
    });
    worker.join();

    REQUIRE(offThreadReady.has_value());
    REQUIRE_FALSE(*offThreadReady);
}
