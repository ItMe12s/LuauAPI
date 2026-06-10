#include "core/Runtime.hpp"
#include "framework/Binding.hpp"

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
