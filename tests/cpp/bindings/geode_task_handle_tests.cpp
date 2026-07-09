#include "bindings/geode/GeodeTaskHandleBinding.hpp"
#include "core/Runtime.hpp"
#include "framework/stack/Stack.hpp"
#include "host/lua_test_helpers.hpp"

#include <arc/task/Task.hpp>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <optional>
#include <string>

namespace {
    struct RuntimeGuard : luauapi_test::RuntimeGuard {
        ~RuntimeGuard() {
            luax::clearGeodeTaskHandles();
        }
    };

    void pushBool(lua_State* L, void const* raw) {
        luax::push(L, *static_cast<bool const*>(raw));
    }

    bool globalBool(lua_State* L, char const* name) {
        lua_getglobal(L, name);
        bool value = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return value;
    }

    std::string globalString(lua_State* L, char const* name) {
        lua_getglobal(L, name);
        size_t len = 0;
        char const* value = lua_tolstring(L, -1, &len);
        std::string out = value ? std::string(value, len) : std::string();
        lua_pop(L, 1);
        return out;
    }

    double globalNumber(lua_State* L, char const* name) {
        lua_getglobal(L, name);
        double value = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return value;
    }

    template <class T>
    std::shared_ptr<arc::TaskState<T>> pushHandle(lua_State* L, char const* name) {
        auto state = std::make_shared<arc::TaskState<T>>();
        luax::pushGeodeTaskHandle<T>(L, arc::TaskHandle<T>(state), &pushBool);
        lua_setglobal(L, name);
        return state;
    }

    std::shared_ptr<arc::TaskState<void>> pushVoidHandle(lua_State* L, char const* name) {
        auto state = std::make_shared<arc::TaskState<void>>();
        luax::pushGeodeTaskHandle<void>(L, arc::TaskHandle<void>(state), nullptr);
        lua_setglobal(L, name);
        return state;
    }

    int takeVoidHandle(lua_State* L) {
        (void)luax::takeGeodeTaskHandle<void>(L, 1, "takeVoid");
        return 0;
    }
} // namespace

TEST_CASE("GeodeTaskHandle completes bool callbacks and late callbacks") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = pushHandle<bool>(L, "h");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.hits = 0
            h:onComplete(function(value, err)
                _G.hits = _G.hits + 1
                _G.doneValue = value
                _G.doneErr = err
            end)
        )"
        )
    );

    luax::pollGeodeTaskHandles(L);
    REQUIRE(globalNumber(L, "hits") == 0.0);

    state->value = true;
    state->ready = true;
    luax::pollGeodeTaskHandles(L);

    REQUIRE(globalNumber(L, "hits") == 1.0);
    REQUIRE(globalBool(L, "doneValue"));
    lua_getglobal(L, "doneErr");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.late = false
            h:onComplete(function(value, err)
                _G.late = value and err == nil
            end)
        )"
        )
    );
    REQUIRE(globalBool(L, "late"));
}

TEST_CASE("GeodeTaskHandle preserves callback order") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = pushHandle<bool>(L, "h");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.order = ""
            h:onComplete(function() _G.order = _G.order .. "a" end)
            h:onComplete(function() _G.order = _G.order .. "b" end)
        )"
        )
    );

    state->value = true;
    state->ready = true;
    luax::pollGeodeTaskHandles(L);
    REQUIRE(globalString(L, "order") == "ab");
}

TEST_CASE("GeodeTaskHandle detach inside callback does not skip queued callbacks") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = pushHandle<bool>(L, "h");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.order = ""
            _G.secondValue = false
            h:onComplete(function()
                _G.order = _G.order .. "a"
                h:detach()
                _G.detachedInside = h:isDetached()
            end)
            h:onComplete(function(value, err)
                _G.order = _G.order .. "b"
                _G.secondValue = value and err == nil
            end)
        )"
        )
    );

    state->value = true;
    state->ready = true;
    luax::pollGeodeTaskHandles(L);
    REQUIRE(globalString(L, "order") == "ab");
    REQUIRE(globalBool(L, "detachedInside"));
    REQUIRE(globalBool(L, "secondValue"));
}

TEST_CASE("GeodeTaskHandle completes void callbacks with nil pair") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = pushVoidHandle(L, "h");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.voidOk = false
            h:onComplete(function(value, err)
                _G.voidOk = value == nil and err == nil
            end)
        )"
        )
    );

    state->ready = true;
    luax::pollGeodeTaskHandles(L);
    REQUIRE(globalBool(L, "voidOk"));
}

TEST_CASE("GeodeTaskHandle cancel aborts and clears callbacks") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = pushHandle<bool>(L, "h");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.cancelHits = 0
            h:onComplete(function() _G.cancelHits = _G.cancelHits + 1 end)
            h:cancel()
            _G.detachedAfterCancel = h:isDetached()
        )"
        )
    );

    REQUIRE(state->aborted);
    REQUIRE(globalBool(L, "detachedAfterCancel"));
    state->value = true;
    state->ready = true;
    luax::pollGeodeTaskHandles(L);
    REQUIRE(globalNumber(L, "cancelHits") == 0.0);
}

TEST_CASE("GeodeTaskHandle detach drops observation without aborting") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = pushHandle<bool>(L, "h");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.detachHits = 0
            h:onComplete(function() _G.detachHits = _G.detachHits + 1 end)
            h:detach()
            _G.detached = h:isDetached()
        )"
        )
    );

    REQUIRE(state->detached);
    REQUIRE_FALSE(state->aborted);
    REQUIRE(globalBool(L, "detached"));
    state->value = true;
    state->ready = true;
    luax::pollGeodeTaskHandles(L);
    REQUIRE(globalNumber(L, "detachHits") == 0.0);
}

TEST_CASE("GeodeTaskHandle reports poll exception as nil err") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = pushHandle<bool>(L, "h");

    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.errText = ""
            _G.errValueWasNil = false
            h:onComplete(function(value, err)
                _G.errValueWasNil = value == nil
                _G.errText = err
            end)
        )"
        )
    );

    state->error = "boom";
    luax::pollGeodeTaskHandles(L);
    REQUIRE(globalBool(L, "errValueWasNil"));
    REQUIRE(globalString(L, "errText") == "boom");
}

TEST_CASE("GeodeTaskHandle optional nil pushes nil") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    luax::pushOptionalGeodeTaskHandle<bool>(L, std::nullopt, &pushBool);
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
}

TEST_CASE("GeodeTaskHandle optional nil arg becomes nullopt") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    lua_pushnil(L);
    auto handle = luax::takeOptionalGeodeTaskHandle<bool>(L, -1, "optional");
    REQUIRE_FALSE(handle.has_value());
    lua_pop(L, 1);
}

TEST_CASE("GeodeTaskHandle consume moves native handle and detaches userdata") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = std::make_shared<arc::TaskState<bool>>();
    luax::pushGeodeTaskHandle<bool>(L, arc::TaskHandle<bool>(state), &pushBool);

    auto native = luax::takeGeodeTaskHandle<bool>(L, -1, "consume");
    lua_setglobal(L, "h");

    REQUIRE(luauapi_test::runScriptVoid(L, "_G.consumedDetached = h:isDetached()"));
    REQUIRE(globalBool(L, "consumedDetached"));

    state->value = true;
    state->ready = true;
    auto waker = arc::Waker::noop();
    arc::Context cx(&waker);
    auto result = native.poll(cx);
    REQUIRE(result);
    REQUIRE(*result);
}

TEST_CASE("GeodeTaskHandle consume rejects mismatched result type") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = std::make_shared<arc::TaskState<bool>>();
    luax::pushGeodeTaskHandle<bool>(L, arc::TaskHandle<bool>(state), &pushBool);
    lua_pushcfunction(L, &takeVoidHandle, "takeVoid");
    lua_insert(L, -2);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    lua_pop(L, 1);
}

TEST_CASE("GeodeTaskHandle clear detaches pending handles on shutdown cleanup") {
    RuntimeGuard guard;
    auto* L = luax::Runtime::getOrCreate()->state();
    auto state = pushHandle<bool>(L, "h");

    luax::clearGeodeTaskHandles();
    REQUIRE(state->detached);
    REQUIRE_FALSE(state->aborted);
}
