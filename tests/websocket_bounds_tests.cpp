#include "bindings/websocket/WebSocketInternal.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "host/lua_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <optional>
#include <string>
#include <thread>

namespace luax {
    geode::Result<void> registerWebSocket(lua_State* L);
} // namespace luax

namespace {
    using namespace luax;
    using luauapi_test::compile;
    using luauapi_test::makeLuaState;

    struct WebSocketGuard {
        WebSocketGuard() {
            Runtime::setMainThreadId(std::this_thread::get_id());
            resetBindingsForTests();
        }

        ~WebSocketGuard() {
            clearWsState();
            Runtime::resetForTests();
            resetBindingsForTests();
        }
    };

    void registerWebSocketBindings(lua_State* L) {
        REQUIRE(registerWebSocket(L).isOk());
    }

    bool runScriptReturnsBool(lua_State* L, std::string const& source) {
        auto bytecode = compile(source);
        if (luau_load(L, "=websocket_bounds_test", bytecode.data(), bytecode.size(), 0) != 0) {
            return false;
        }
        if (lua_pcall(L, 0, 1, 0) != 0) {
            return false;
        }
        if (!lua_isboolean(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        bool const value = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return value;
    }

    std::optional<std::string> runScriptReturnsString(lua_State* L, std::string const& source) {
        auto bytecode = compile(source);
        if (luau_load(L, "=websocket_bounds_test", bytecode.data(), bytecode.size(), 0) != 0) {
            return std::nullopt;
        }
        if (lua_pcall(L, 0, 1, 0) != 0) {
            return std::nullopt;
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        size_t len = 0;
        char const* text = lua_tolstring(L, -1, &len);
        std::string value(text ? text : "", len);
        lua_pop(L, 1);
        return value;
    }

    std::size_t countLiveConnections() {
        return activeWsConnections().compactAndCountLive();
    }

    std::size_t countLiveServers() {
        return activeWsServers().compactAndCountLive();
    }
} // namespace

TEST_CASE("websocket.connect rejects non-ws schemes") {
    WebSocketGuard guard;
    auto L = makeLuaState(true);
    registerWebSocketBindings(L.get());

    auto err = runScriptReturnsString(L.get(), R"(
        local conn, err = websocket.connect("http://127.0.0.1:9")
        return err
    )");
    REQUIRE(err.has_value());
    REQUIRE(err->find("ws://") != std::string::npos);
}

TEST_CASE("websocket.connect rejects connection cap overflow") {
    WebSocketGuard guard;
    auto L = makeLuaState(true);
    registerWebSocketBindings(L.get());
    Runtime::getOrCreate();

    REQUIRE(runScriptReturnsBool(
        L.get(),
        std::string(R"(
        local conns = {}
        for i = 1, )") +
            std::to_string(kMaxWebSocketConnections) + R"( do
            local conn, err = websocket.connect("ws://127.0.0.1:9")
            if not conn then
                return false
            end
            conns[i] = conn
        end
        local conn, err = websocket.connect("ws://127.0.0.1:9")
        return conn == nil and err ~= nil and string.find(err, "too many") ~= nil
    )"
    ));
}

TEST_CASE("websocket connection send rejects oversized payload") {
    WebSocketGuard guard;
    auto L = makeLuaState(true);
    registerWebSocketBindings(L.get());
    Runtime::getOrCreate();

    REQUIRE(runScriptReturnsBool(
        L.get(),
        std::string(R"(
        local conn = websocket.connect("ws://127.0.0.1:9")
        if not conn then
            return false
        end
        local ok, err = conn:send(string.rep("x", )") +
            std::to_string(kMaxWebSocketSendBytes + 1) +
            R"( + 1))
        return ok == nil and err == "websocket message exceeds maximum send size"
    )"
    ));
}

TEST_CASE("websocket.serve rejects invalid ports") {
    WebSocketGuard guard;
    auto L = makeLuaState(true);
    registerWebSocketBindings(L.get());

    auto errZero = runScriptReturnsString(L.get(), R"(
        local srv, err = websocket.serve(0)
        return err
    )");
    REQUIRE(errZero.has_value());
    REQUIRE(errZero->find("1..65535") != std::string::npos);

    auto errHigh = runScriptReturnsString(L.get(), R"(
        local srv, err = websocket.serve(70000)
        return err
    )");
    REQUIRE(errHigh.has_value());
    REQUIRE(errHigh->find("1..65535") != std::string::npos);
}

TEST_CASE("websocket.serve rejects server cap overflow") {
    WebSocketGuard guard;
    auto L = makeLuaState(true);
    registerWebSocketBindings(L.get());
    Runtime::getOrCreate();

    REQUIRE(runScriptReturnsBool(L.get(), R"(
        local first = websocket.serve(59123)
        local second = websocket.serve(59124)
        if not first or not second then
            return false
        end
        local third, err = websocket.serve(59125)
        return third == nil and err ~= nil and string.find(err, "too many") ~= nil
    )"));
}

TEST_CASE("websocket server broadcast rejects oversized payload") {
    WebSocketGuard guard;
    auto L = makeLuaState(true);
    registerWebSocketBindings(L.get());
    Runtime::getOrCreate();

    REQUIRE(runScriptReturnsBool(
        L.get(),
        std::string(R"(
        local srv = websocket.serve(59126)
        if not srv then
            return false
        end
        local ok, err = srv:broadcast(string.rep("x", )") +
            std::to_string(kMaxWebSocketSendBytes + 1) +
            R"( + 1))
        return ok == nil and err == "websocket message exceeds maximum send size"
    )"
    ));
}

TEST_CASE("runtime reset clears global websocket state") {
    WebSocketGuard guard;
    auto L = makeLuaState(true);
    registerWebSocketBindings(L.get());
    Runtime::getOrCreate();

    REQUIRE(runScriptReturnsBool(L.get(), R"(
        local conn = websocket.connect("ws://127.0.0.1:9")
        local srv = websocket.serve(59127)
        return conn ~= nil and srv ~= nil
    )"));
    REQUIRE(countLiveConnections() >= 1);
    REQUIRE(countLiveServers() >= 1);
    REQUIRE(wsShutdownHookRegistered());

    Runtime::resetForTests();
    Runtime::setMainThreadId(std::this_thread::get_id());

    REQUIRE(countLiveConnections() == 0);
    REQUIRE(countLiveServers() == 0);
    REQUIRE_FALSE(wsShutdownHookRegistered());
}
