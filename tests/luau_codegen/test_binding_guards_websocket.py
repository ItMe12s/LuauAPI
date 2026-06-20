from __future__ import annotations

import unittest

from binding_guard_support import (
    CONFIG_HEADER,
    WEB_INTERNAL,
    WEBSOCKET_CONNECTION,
    WEBSOCKET_INTERNAL,
    WEBSOCKET_SERVER,
    function_body,
    inline_function_body,
    read_repo_file,
)


class WebSocketGuardTests(unittest.TestCase):
    def test_websocket_constants_are_declared(self) -> None:
        config = read_repo_file(CONFIG_HEADER)
        for name in (
            "kMaxWebSocketConnections",
            "kMaxWebSocketServers",
            "kMaxWebSocketServerClients",
            "kMaxWebSocketSendBytes",
            "kMaxWebSocketReceiveBytes",
        ):
            with self.subTest(name=name):
                self.assertIn(name, config)

    def test_ws_connect_enforces_connection_cap(self) -> None:
        source = read_repo_file(WEBSOCKET_CONNECTION)
        body = function_body(source, "wsConnect")
        self.assertIn(
            "kMaxWebSocketConnections",
            body,
            "wsConnect must enforce kMaxWebSocketConnections",
        )

    def test_ws_serve_enforces_server_cap_and_client_limit(self) -> None:
        source = read_repo_file(WEBSOCKET_SERVER)
        body = function_body(source, "wsServe")
        self.assertIn(
            "kMaxWebSocketServers",
            body,
            "wsServe must enforce kMaxWebSocketServers",
        )
        self.assertIn(
            "kMaxWebSocketServerClients",
            body,
            "wsServe must pass kMaxWebSocketServerClients to server construction",
        )

    def test_websocket_send_methods_enforce_size_cap(self) -> None:
        internal = read_repo_file(WEBSOCKET_INTERNAL)
        self.assertIn("kMaxWebSocketSendBytes", internal)
        self.assertIn("wsSendWithinLimit", internal)

        for rel_path, fn in (
            (WEBSOCKET_CONNECTION, "sendData"),
            (WEBSOCKET_SERVER, "broadcastData"),
            (WEBSOCKET_SERVER, "peerSendData"),
        ):
            with self.subTest(fn=fn):
                source = read_repo_file(rel_path)
                body = function_body(source, fn)
                self.assertIn(
                    "wsSendWithinLimit",
                    body,
                    f"{fn} must enforce send size via wsSendWithinLimit",
                )

    def test_websocket_peer_send_delegates_to_peer_send_data(self) -> None:
        source = read_repo_file(WEBSOCKET_SERVER)
        for fn in ("peerSend", "peerSendBinary"):
            with self.subTest(fn=fn):
                body = function_body(source, fn)
                self.assertIn(
                    "peerSendData",
                    body,
                    f"{fn} must delegate to peerSendData for size enforcement",
                )

    def test_websocket_message_callbacks_enforce_receive_cap(self) -> None:
        conn_source = read_repo_file(WEBSOCKET_CONNECTION)
        conn_body = function_body(conn_source, "installMessageCallback", ret="void")
        self.assertIn(
            "kMaxWebSocketReceiveBytes",
            conn_body,
            "client message callback must enforce kMaxWebSocketReceiveBytes",
        )

        server_source = read_repo_file(WEBSOCKET_SERVER)
        server_body = function_body(server_source, "handleServerClientMessage", ret="void")
        self.assertIn(
            "kMaxWebSocketReceiveBytes",
            server_body,
            "server message callback must enforce kMaxWebSocketReceiveBytes",
        )

    def test_websocket_callbacks_queue_to_main_thread(self) -> None:
        internal = read_repo_file(WEBSOCKET_INTERNAL)
        start = internal.find("void queueWsEvent(")
        self.assertNotEqual(start, -1, "missing shared queueWsEvent helper")
        end = internal.find("inline constexpr char kWsConnectionClosedMsg", start)
        template_body = internal[start:end]
        self.assertIn(
            "geode::queueInMainThread",
            template_body,
            "queueWsEvent must defer websocket callbacks to the main thread",
        )
        self.assertIn(
            "stopped.load()",
            template_body,
            "queueWsEvent must drop events for stopped owners",
        )

        conn_source = read_repo_file(WEBSOCKET_CONNECTION)
        open_body = function_body(conn_source, "queueOpenEvent", ret="void")
        self.assertIn(
            "queueInMainThread",
            open_body,
            "queueOpenEvent must defer to the main thread",
        )
        self.assertGreaterEqual(
            conn_source.count("queueWsEvent<"),
            3,
            "connection message/close/error events must route through queueWsEvent",
        )

        server_source = read_repo_file(WEBSOCKET_SERVER)
        self.assertGreaterEqual(
            server_source.count("queueWsEvent<"),
            4,
            "server connect/message/disconnect/error events must route through queueWsEvent",
        )

    def test_websocket_shutdown_hook_clears_state(self) -> None:
        source = read_repo_file(WEBSOCKET_INTERNAL)
        self.assertIn("clearWsState", source)
        self.assertIn("WeakHandlePool", source)
        self.assertIn("ensureShutdownHook(wsShutdownHookRegistered()", source)
        body = inline_function_body(source, "inline void clearWsState()")
        self.assertIn("activeWsConnections().clearAll", body)
        self.assertIn("activeWsServers().clearAll", body)

    def test_web_shutdown_hook_clears_state(self) -> None:
        source = read_repo_file(WEB_INTERNAL)
        self.assertIn("clearWebState", source)
        self.assertIn("WeakHandlePool", source)
        self.assertIn("ensureShutdownHook(webShutdownHookRegistered()", source)
        body = inline_function_body(source, "inline void clearWebState()")
        self.assertIn("activeTasks().clearAll", body)
        self.assertIn("activeListeners().clearAll", body)
