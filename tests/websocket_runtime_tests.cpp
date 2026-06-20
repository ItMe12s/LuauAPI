#include "bindings/websocket/WebSocketInternal.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "host/lua_test_helpers.hpp"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <lua.h>
#include <string>
#include <thread>

namespace luax {
    geode::Result<void> registerWebSocket(lua_State* L);
} // namespace luax

namespace {
    using namespace luax;
    using Clock = std::chrono::steady_clock;
    using luauapi_test::collectGarbage;

    std::atomic<int> g_portCounter{0};

    int nextPort() {
        return 59200 + (g_portCounter.fetch_add(1) % 800);
    }

    using WebSocketRuntimeGuard = luauapi_test::WebSocketRuntimeGuard;

    struct WsRuntimeFixture {
        lua_State* L = nullptr;

        WsRuntimeFixture() {
            auto* runtime = Runtime::getOrCreate();
            L = runtime->state();
            REQUIRE(L != nullptr);
            REQUIRE(registerWebSocket(L).isOk());
        }
    };

    bool waitUntil(std::function<bool()> pred, int timeoutMs = 5000) {
        auto const deadline = Clock::now() + std::chrono::milliseconds(timeoutMs);
        while (Clock::now() < deadline) {
            geode::test::drainMainThreadQueue();
            if (pred()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        geode::test::drainMainThreadQueue();
        return pred();
    }

    bool runScript(lua_State* L, std::string const& source) {
        return luauapi_test::runScriptVoid(L, source, "=websocket_runtime_test");
    }

    bool runScriptReturnsBool(lua_State* L, std::string const& source) {
        return luauapi_test::runScriptReturnsBool(L, source, "=websocket_runtime_test");
    }

    bool evalBool(lua_State* L, std::string const& expression) {
        return runScriptReturnsBool(L, "return " + expression);
    }

    std::string wsTestStateInit() {
        return R"(
        _ws_test = {
            opened = false,
            message = nil,
            binary = nil,
            clientCount = 0,
            peerId = nil,
            closeCode = nil,
            closeReason = nil,
            closeRemote = nil,
            disconnectCode = nil,
            disconnectReason = nil,
            pingOk = nil,
            pingErr = nil,
            peer = nil,
        }
    )";
    }

    std::string serverOnlySetupScript(int port) {
        return wsTestStateInit() +
            R"(
        local port = )" +
            std::to_string(port) +
            R"(
        _ws_server = websocket.serve(port)
        return _ws_server ~= nil
    )";
    }

    std::string serverEchoCallbacksScript() {
        return R"(
        _ws_server:onMessage(function(peer, data, isBinary)
            if isBinary then
                peer:sendBinary(data)
            else
                peer:send(data)
            end
        end)
    )";
    }

    std::string connectUrl(int port) {
        return "ws://127.0.0.1:" + std::to_string(port);
    }

    std::string connectClientScript(int port) {
        return R"(
        _ws_client = websocket.connect(")" +
            connectUrl(port) +
            R"(")
        return _ws_client ~= nil
    )";
    }

    std::string connectClientTextEchoCallbacksScript() {
        return R"(
        _ws_client:onOpen(function()
            _ws_test.opened = true
            _ws_client:send("hello")
        end):onMessage(function(data, isBinary)
            _ws_test.message = data
            _ws_test.binary = isBinary
        end)
    )";
    }

    std::string connectClientBinaryEchoCallbacksScript() {
        return R"(
        _ws_client:onOpen(function()
            _ws_test.opened = true
            _ws_client:sendBinary(string.char(1, 2, 3))
        end):onMessage(function(data, isBinary)
            _ws_test.message = data
            _ws_test.binary = isBinary
        end)
    )";
    }

    std::string connectClientWelcomeCallbacksScript() {
        return R"(
        _ws_client:onMessage(function(data, isBinary)
            _ws_test.message = data
            _ws_test.binary = isBinary
        end)
    )";
    }

    std::string connectClientCloseCallbacksScript() {
        return R"(
        _ws_client:onOpen(function()
            _ws_test.opened = true
        end):onClose(function(code, reason, remote)
            _ws_test.closeCode = code
            _ws_test.closeReason = reason
            _ws_test.closeRemote = remote
        end)
    )";
    }

    std::string connectClientPingCallbacksScript() {
        return R"(
        _ws_client:onOpen(function()
            _ws_test.opened = true
            local ok, err = _ws_client:ping("probe")
            _ws_test.pingOk = ok
            _ws_test.pingErr = err
        end)
    )";
    }

    std::string gcSurvivalServerCallbacksScript() {
        return R"(
        _ws_server:onClientConnect(function() end)
        _ws_server:onMessage(function() end)
        _ws_server:onClientDisconnect(function() end)
        _ws_server:onError(function() end)
    )";
    }

    std::string gcSurvivalClientCallbacksScript() {
        return R"(
        _ws_client:onOpen(function() end)
        _ws_client:onMessage(function() end)
        _ws_client:onClose(function() end)
        _ws_client:onError(function() end)
    )";
    }

    std::string oversizeLocalDecl() {
        return "local oversize = " + std::to_string(kMaxWebSocketSendBytes + 1) + "\n";
    }
} // namespace

TEST_CASE("websocket loopback text echo round-trip") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, serverOnlySetupScript(port)));
    REQUIRE(runScript(fixture.L, serverEchoCallbacksScript()));
    REQUIRE(runScriptReturnsBool(fixture.L, connectClientScript(port)));
    REQUIRE(runScript(fixture.L, connectClientTextEchoCallbacksScript()));

    REQUIRE(waitUntil([&] {
        return evalBool(
            fixture.L,
            "_ws_test.opened == true and _ws_test.message == 'hello' and _ws_test.binary == false"
        );
    }));
}

TEST_CASE("websocket loopback binary round-trip") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, serverOnlySetupScript(port)));
    REQUIRE(runScript(fixture.L, serverEchoCallbacksScript()));
    REQUIRE(runScriptReturnsBool(fixture.L, connectClientScript(port)));
    REQUIRE(runScript(fixture.L, connectClientBinaryEchoCallbacksScript()));

    REQUIRE(waitUntil([&] {
        return evalBool(
            fixture.L,
            "_ws_test.opened == true and _ws_test.message == string.char(1, 2, 3) and "
            "_ws_test.binary == true"
        );
    }));
}

TEST_CASE("websocket callback registration survives garbage collection") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, serverOnlySetupScript(port)));
    REQUIRE(runScript(fixture.L, gcSurvivalServerCallbacksScript()));
    REQUIRE(runScriptReturnsBool(fixture.L, connectClientScript(port)));
    REQUIRE(runScript(fixture.L, gcSurvivalClientCallbacksScript()));
    collectGarbage(fixture.L);
    collectGarbage(fixture.L);
}

TEST_CASE("websocket server peer connect send and clients") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, serverOnlySetupScript(port)));

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_server:onMessage(function(peer, data, isBinary)
            if isBinary then
                peer:sendBinary(data)
            else
                peer:send(data)
            end
        end):onClientConnect(function(peer, headers)
            _ws_test.clientCount = #_ws_server:clients()
            _ws_test.peerId = peer:id()
            peer:send("welcome")
        end)
    )"
    ));

    REQUIRE(runScriptReturnsBool(fixture.L, connectClientScript(port)));
    REQUIRE(runScript(fixture.L, connectClientWelcomeCallbacksScript()));

    REQUIRE(waitUntil([&] {
        return evalBool(
            fixture.L,
            "_ws_test.clientCount == 1 and type(_ws_test.peerId) == 'string' and "
            "_ws_test.message == 'welcome' and _ws_test.binary == false"
        );
    }));
}

TEST_CASE("websocket connection sendBinary rejects oversized payload") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        oversizeLocalDecl() +
            R"(
        local conn = websocket.connect(")" +
            connectUrl(port) +
            R"(")
        if not conn then
            return false
        end
        local ok, err = conn:sendBinary(string.rep('x', oversize))
        return ok == nil and err == "websocket message exceeds maximum send size"
    )"
    ));
}

TEST_CASE("websocket server broadcastBinary rejects oversized payload") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        oversizeLocalDecl() +
            R"(
        local port = )" +
            std::to_string(port) +
            R"(
        local srv = websocket.serve(port)
        if not srv then
            return false
        end
        local ok, err = srv:broadcastBinary(string.rep('x', oversize))
        return ok == nil and err == "websocket message exceeds maximum send size"
    )"
    ));
}

TEST_CASE("websocket peer send rejects oversized payload") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, serverOnlySetupScript(port)));

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_server:onClientConnect(function(peer)
            _ws_test.peer = peer
            _ws_test.opened = true
        end)
    )"
    ));

    REQUIRE(runScriptReturnsBool(fixture.L, connectClientScript(port)));

    REQUIRE(waitUntil([&] {
        return evalBool(fixture.L, "_ws_test.opened == true");
    }));

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        oversizeLocalDecl() +
            R"(
        local ok, err = _ws_test.peer:send(string.rep('x', oversize))
        return ok == nil and err == "websocket message exceeds maximum send size"
    )"
    ));
}

TEST_CASE("websocket close and disconnect populate callback codes") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, serverOnlySetupScript(port)));

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_server:onClientDisconnect(function(peer, code, reason)
            _ws_test.disconnectCode = code
            _ws_test.disconnectReason = reason
        end)
    )"
    ));

    REQUIRE(runScriptReturnsBool(fixture.L, connectClientScript(port)));
    REQUIRE(runScript(fixture.L, connectClientCloseCallbacksScript()));

    REQUIRE(waitUntil([&] {
        return evalBool(fixture.L, "_ws_test.opened == true");
    }));

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_client:close(1000, "bye")
    )"
    ));

    REQUIRE(waitUntil([&] {
        return evalBool(
            fixture.L,
            "_ws_test.closeCode == 1000 and _ws_test.closeReason == 'bye' and "
            "type(_ws_test.closeRemote) == 'boolean' and _ws_test.disconnectCode == 1000"
        );
    }));
}

TEST_CASE("websocket ping returns boolean error shape") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, serverOnlySetupScript(port)));
    REQUIRE(runScript(fixture.L, serverEchoCallbacksScript()));
    REQUIRE(runScriptReturnsBool(fixture.L, connectClientScript(port)));
    REQUIRE(runScript(fixture.L, connectClientPingCallbacksScript()));

    REQUIRE(waitUntil([&] {
        return evalBool(
            fixture.L,
            "_ws_test.opened == true and _ws_test.pingOk == true and _ws_test.pingErr == nil"
        );
    }));

    REQUIRE(runScript(fixture.L, R"(
        _ws_client:close()
    )"));

    REQUIRE(waitUntil([&] {
        return runScriptReturnsBool(
            fixture.L,
            R"(
            local ok, err = _ws_client:ping()
            return ok == nil and type(err) == "string"
        )"
        );
    }));
}
