#include "lua/bindings/framework/LuaCallback.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"
#include "lua_test_helpers.hpp"

#include <Geode/log.hpp>

#include <catch2/catch_test_macros.hpp>
#include <lua.h>

#include <functional>
#include <memory>
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

    std::function<void()> makeCodegenVoidCallback(lua_State* L, int fnIndex) {
        auto cb_cb = std::make_shared<luax::LuaCallback>(L, fnIndex);
        return [cb_cb]() {
            if (!cb_cb->invoke(0, 0, "test callback", luax::kHookScriptDeadlineMs)) {
                geode::log::error("test callback failed");
                return;
            }
        };
    }

    std::function<bool()> makeCodegenBoolCallback(lua_State* L, int fnIndex) {
        auto cb_cb = std::make_shared<luax::LuaCallback>(L, fnIndex);
        return [cb_cb]() -> bool {
            bool cb_ret{};
            if (!cb_cb->invoke(
                    0,
                    1,
                    "test callback",
                    luax::kHookScriptDeadlineMs,
                    nullptr,
                    nullptr,
                    +[](lua_State* L, void* raw) {
                        *static_cast<bool*>(raw) =
                            luax::check<bool>(L, -1, "cb callback return");
                    },
                    &cb_ret
                )) {
                geode::log::error("test callback failed");
                return false;
            }
            return cb_ret;
        };
    }
}

TEST_CASE("Generated-style void std::function callback returns cleanly when invoke fails") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    luauapi_test::loadFunction(L, "error('callback boom')");
    auto cb = makeCodegenVoidCallback(L, -1);
    lua_pop(L, 1);

    REQUIRE_NOTHROW(cb());
}

TEST_CASE("Generated-style bool std::function callback returns false when invoke fails") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    luauapi_test::loadFunction(L, "error('callback boom')");
    auto cb = makeCodegenBoolCallback(L, -1);
    lua_pop(L, 1);

    REQUIRE_FALSE(cb());
}

TEST_CASE("Generated-style bool std::function callback returns Lua result on success") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    luauapi_test::loadFunction(L, "return true");
    auto cb = makeCodegenBoolCallback(L, -1);
    lua_pop(L, 1);

    REQUIRE(cb());
}
