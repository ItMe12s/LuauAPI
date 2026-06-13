#include "bindings/websocket/WebSocketInternal.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "host/lua_test_helpers.hpp"

#include <Geode/utils/main_thread.hpp>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace luax {
    geode::Result<void> registerWebSocket(lua_State* L);
} // namespace luax

namespace {
    using namespace luax;
    using Clock = std::chrono::steady_clock;

    std::atomic<int> g_portCounter{0};

    int nextPort() {
        return 59200 + (g_portCounter.fetch_add(1) % 800);
    }

    struct WebSocketRuntimeGuard {
        WebSocketRuntimeGuard() {
            Runtime::setMainThreadId(std::this_thread::get_id());
            geode::test::bindMainThreadToCurrent();
            resetBindingsForTests();
        }

        ~WebSocketRuntimeGuard() {
            clearWsState();
            geode::test::clearMainThreadQueue();
            Runtime::resetForTests();
            resetBindingsForTests();
        }
    };

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
        luauapi_test::loadFunction(L, source, "=websocket_runtime_test");
        if (lua_pcall(L, 0, 0, 0) != 0) {
            if (lua_isstring(L, -1)) {
                INFO(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    bool runScriptReturnsBool(lua_State* L, std::string const& source) {
        luauapi_test::loadFunction(L, source, "=websocket_runtime_test");
        if (lua_pcall(L, 0, 1, 0) != 0) {
            if (lua_isstring(L, -1)) {
                INFO(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
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

    std::string serverOnlySetupScript(int port) {
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
            pingOk = nil,
            pingErr = nil,
        }

        local port = )" +
            std::to_string(port) +
            R"(
        _ws_server = websocket.serve(port)
        return _ws_server ~= nil
    )";
    }

    std::string connectClientScript(int port) {
        return R"(
        _ws_client = websocket.connect("ws://127.0.0.1:)" +
            std::to_string(port) +
            R"(")
        return _ws_client ~= nil
    )";
    }

    std::string oversizeLocalDecl() {
        return "local oversize = " + std::to_string(kMaxWebSocketSendBytes + 1) + " + 1\n";
    }

    std::string echoSetupScript(int port) {
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
            pingOk = nil,
            pingErr = nil,
        }

        local port = )" +
            std::to_string(port) +
            R"(
        _ws_server = websocket.serve(port)
        if not _ws_server then
            return false
        end

        _ws_server:onMessage(function(peer, data, isBinary)
            if isBinary then
                peer:sendBinary(data)
            else
                peer:send(data)
            end
        end)

        _ws_client = websocket.connect("ws://127.0.0.1:" .. tostring(port))
        if not _ws_client then
            return false
        end

        return true
    )";
    }
} // namespace

TEST_CASE("websocket loopback text echo round-trip") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, echoSetupScript(port)));

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_client:onOpen(function()
            _ws_test.opened = true
            _ws_client:send("hello")
        end):onMessage(function(data, isBinary)
            _ws_test.message = data
            _ws_test.binary = isBinary
        end)
    )"
    ));

    REQUIRE(waitUntil([&] {
        return runScriptReturnsBool(
            fixture.L,
            "_ws_test.opened == true and _ws_test.message == 'hello' and _ws_test.binary == false"
        );
    }));
}

TEST_CASE("websocket loopback binary round-trip") {
    WebSocketRuntimeGuard guard;
    WsRuntimeFixture fixture;
    int const port = nextPort();

    REQUIRE(runScriptReturnsBool(fixture.L, echoSetupScript(port)));

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_client:onOpen(function()
            _ws_test.opened = true
            _ws_client:sendBinary(string.char(1, 2, 3))
        end):onMessage(function(data, isBinary)
            _ws_test.message = data
            _ws_test.binary = isBinary
        end)
    )"
    ));

    REQUIRE(waitUntil([&] {
        return runScriptReturnsBool(
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

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        std::string(R"(
        local server = websocket.serve()") +
            std::to_string(port) +
            R"(
        if not server then
            return false
        end

        local client = websocket.connect("ws://127.0.0.1:)" +
            std::to_string(port) +
            R"(")
        if not client then
            return false
        end

        client:onOpen(function() end)
            :onMessage(function() end)
            :onClose(function() end)
            :onError(function() end)

        server:onClientConnect(function() end)
            :onMessage(function() end)
            :onClientDisconnect(function() end)
            :onError(function() end)

        client = nil
        server = nil
        collectgarbage("collect")
        collectgarbage("collect")
        return true
    )"
    ));
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

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_client:onMessage(function(data, isBinary)
            _ws_test.message = data
            _ws_test.binary = isBinary
        end)
    )"
    ));

    REQUIRE(waitUntil([&] {
        return runScriptReturnsBool(
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
        local conn = websocket.connect("ws://127.0.0.1:)" +
            std::to_string(port) +
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
        return runScriptReturnsBool(fixture.L, "_ws_test.opened == true");
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

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_client:onOpen(function()
            _ws_test.opened = true
        end):onClose(function(code, reason, remote)
            _ws_test.closeCode = code
            _ws_test.closeReason = reason
            _ws_test.closeRemote = remote
        end)
    )"
    ));

    REQUIRE(waitUntil([&] {
        return runScriptReturnsBool(fixture.L, "_ws_test.opened == true");
    }));

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_client:close(1000, "bye")
    )"
    ));

    REQUIRE(waitUntil([&] {
        return runScriptReturnsBool(
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

    REQUIRE(runScriptReturnsBool(fixture.L, echoSetupScript(port)));

    REQUIRE(runScript(
        fixture.L,
        R"(
        _ws_client:onOpen(function()
            _ws_test.opened = true
            local ok, err = _ws_client:ping("probe")
            _ws_test.pingOk = ok
            _ws_test.pingErr = err
        end)
    )"
    ));

    REQUIRE(waitUntil([&] {
        return runScriptReturnsBool(
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
