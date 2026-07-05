#include "bindings/websocket/WebSocketInternal.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/string.hpp>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <string>
#include <utility>

namespace luax::wsdetail {
    using namespace luax;

    namespace {
        constexpr char const* kMethod = "websocket.connect";

        struct MessageCtx {
            std::string data;
            bool binary;
        };

        struct CloseCtx {
            std::uint16_t code;
            std::string reason;
            bool remote;
        };

        void pushMessageArgs(lua_State* L, MessageCtx const* ctx) {
            push(L, ctx->data);
            push(L, ctx->binary);
        }

        void pushCloseArgs(lua_State* L, CloseCtx const* ctx) {
            push(L, static_cast<int>(ctx->code));
            push(L, ctx->reason);
            push(L, ctx->remote);
        }

        void pushErrorArgs(lua_State* L, std::string const* ctx) {
            push(L, *ctx);
        }

        void queueOpenEvent(std::weak_ptr<WsConnection> weak) {
            geode::queueInMainThread([weak = std::move(weak)] {
                auto conn = weak.lock();
                if (!conn || conn->stopped.load()) return;
                invokeWsCallback(
                    conn->slot(&WsConnection::onOpen), "websocket.onOpen", 0, nullptr, nullptr
                );
            });
        }

        void queueMessageEvent(std::weak_ptr<WsConnection> weak, std::string data, bool binary) {
            queueWsEvent<WsConnection, MessageCtx, pushMessageArgs>(
                std::move(weak),
                &WsConnection::onMessage,
                "websocket.onMessage",
                2,
                MessageCtx{std::move(data), binary}
            );
        }

        void queueCloseEvent(
            std::weak_ptr<WsConnection> weak, std::uint16_t code, std::string reason, bool remote
        ) {
            queueWsEvent<WsConnection, CloseCtx, pushCloseArgs>(
                std::move(weak),
                &WsConnection::onClose,
                "websocket.onClose",
                3,
                CloseCtx{code, std::move(reason), remote}
            );
        }

        void queueErrorEvent(std::weak_ptr<WsConnection> weak, std::string message) {
            queueWsEvent<WsConnection, std::string, pushErrorArgs>(
                std::move(weak), &WsConnection::onError, "websocket.onError", 1, std::move(message)
            );
        }

        void installMessageCallback(std::shared_ptr<WsConnection> const& conn) {
            std::weak_ptr<WsConnection> weak = conn;
            conn->socket.setOnMessageCallback([weak](ix::WebSocketMessagePtr const& msg) {
                auto conn = weak.lock();
                if (!conn || conn->stopped.load()) return;
                switch (msg->type) {
                    case ix::WebSocketMessageType::Open: queueOpenEvent(weak); break;
                    case ix::WebSocketMessageType::Message:
                        if (msg->str.size() > kMaxWebSocketReceiveBytes) {
                            queueErrorEvent(weak, kWsReceiveSizeExceededMsg);
                            conn->socket.close(1009, "message too big");
                            break;
                        }
                        queueMessageEvent(weak, msg->str, msg->binary);
                        break;
                    case ix::WebSocketMessageType::Close:
                        queueCloseEvent(
                            weak, msg->closeInfo.code, msg->closeInfo.reason, msg->closeInfo.remote
                        );
                        break;
                    case ix::WebSocketMessageType::Error:
                        queueErrorEvent(weak, msg->errorInfo.reason);
                        break;
                    default: break;
                }
            });
        }

        void applyHeaders(lua_State* L, WsConnection& conn, int idx) {
            lua_getfield(L, idx, "headers");
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return;
            }
            luaL_checktype(L, -1, LUA_TTABLE);
            ix::WebSocketHttpHeaders headers;
            int table = lua_absindex(L, -1);
            lua_pushnil(L);
            while (lua_next(L, table) != 0) {
                if (lua_type(L, -2) != LUA_TSTRING) {
                    luaL_error(L, "%s expected string header names", kMethod);
                }
                auto key = check<std::string>(L, -2, kMethod);
                auto value = check<std::string>(L, -1, kMethod);
                headers[std::move(key)] = std::move(value);
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
            if (!headers.empty()) conn.socket.setExtraHeaders(headers);
        }

        void applyProtocols(lua_State* L, WsConnection& conn, int idx) {
            lua_getfield(L, idx, "protocols");
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return;
            }
            luaL_checktype(L, -1, LUA_TTABLE);
            int table = lua_absindex(L, -1);
            int count = lua_objlen(L, table);
            for (int i = 1; i <= count; ++i) {
                lua_rawgeti(L, table, i);
                conn.socket.addSubProtocol(check<std::string>(L, -1, kMethod));
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }

        void applyConnectOptions(lua_State* L, WsConnection& conn, int idx) {
            bool autoReconnect = false;
            if (lua_gettop(L) >= idx && !lua_isnil(L, idx)) {
                luaL_checktype(L, idx, LUA_TTABLE);
                idx = lua_absindex(L, idx);

                applyHeaders(L, conn, idx);
                applyProtocols(L, conn, idx);

                if (auto value = optNumberField(L, idx, "pingIntervalSecs", kMethod)) {
                    if (*value < 1) luaL_error(L, "%s expected pingIntervalSecs >= 1", kMethod);
                    conn.socket.setPingInterval(static_cast<int>(*value));
                }
                if (auto value = optNumberField(L, idx, "handshakeTimeoutSecs", kMethod)) {
                    if (*value < 1) luaL_error(L, "%s expected handshakeTimeoutSecs >= 1", kMethod);
                    conn.socket.setHandshakeTimeout(static_cast<int>(*value));
                }
                if (auto value = optBoolField(L, idx, "autoReconnect", kMethod)) {
                    autoReconnect = *value;
                }

                ix::SocketTLSOptions tls;
                bool tlsSet = false;
                if (auto value = optBoolField(L, idx, "certVerification", kMethod)) {
                    if (!*value) {
                        tls.caFile = "NONE";
                        tlsSet = true;
                    }
                }
                if (auto value = optStringField(L, idx, "caBundle", kMethod)) {
                    tls.caFile = std::move(*value);
                    tlsSet = true;
                }
                if (tlsSet) conn.socket.setTLSOptions(tls);
            }
            if (!autoReconnect) conn.socket.disableAutomaticReconnection();
        }

        WsConnection& checkOpenConnection(lua_State* L) {
            auto* box = checkWsConnection(L, 1);
            if (!box->conn) luaL_error(L, "%s", kWsConnectionClosedMsg);
            return *box->conn;
        }

        int setConnCallback(lua_State* L, std::shared_ptr<LuaCallback> WsConnection::* member) {
            auto& conn = checkOpenConnection(L);
            luaL_checktype(L, 2, LUA_TFUNCTION);
            conn.setSlot(member, std::make_shared<LuaCallback>(L, 2));
            lua_pushvalue(L, 1);
            return 1;
        }

        int sendData(lua_State* L, bool binary) {
            auto& conn = checkOpenConnection(L);
            auto data = check<std::string>(L, 2, binary ? "sendBinary" : "send");
            if (!wsSendWithinLimit(data.size())) {
                return pushWsSendSizeExceeded(L);
            }
            if (conn.stopped.load()) {
                return pushNilErr(L, kWsConnectionClosedMsg);
            }
            auto info = binary ? conn.socket.sendBinary(data) : conn.socket.sendText(data);
            return wsSendResult(L, info, kWsConnectionClosedMsg);
        }
    } // namespace

    int wsConnect(lua_State* L) {
        auto url = check<std::string>(L, 1, kMethod);
        if (!geode::utils::string::startsWith(url, "ws://") &&
            !geode::utils::string::startsWith(url, "wss://")) {
            return pushNilErr(L, "websocket url must start with ws:// or wss://");
        }
        if (activeWsConnections().compactAndCountLive() >= kMaxWebSocketConnections) {
            return pushNilErr(L, "too many websocket connections");
        }

        ensureWsNetSystem();
        auto conn = std::make_shared<WsConnection>();
        conn->socket.setUrl(url);
        applyConnectOptions(L, *conn, 2);
        installMessageCallback(conn);

        activeWsConnections().track(conn);
        ensureWsShutdownHook();
        conn->socket.start();

        pushWsConnection(L, std::move(conn));
        return 1;
    }

    int connSend(lua_State* L) {
        return sendData(L, false);
    }

    int connSendBinary(lua_State* L) {
        return sendData(L, true);
    }

    int connPing(lua_State* L) {
        auto& conn = checkOpenConnection(L);
        std::string payload;
        if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
            payload = check<std::string>(L, 2, "ping");
        }
        if (conn.stopped.load()) {
            return pushNilErr(L, kWsConnectionClosedMsg);
        }
        return wsSendResult(L, conn.socket.ping(payload), kWsConnectionClosedMsg);
    }

    int connClose(lua_State* L) {
        auto& conn = checkOpenConnection(L);
        auto args = parseCloseArgs(L);
        conn.socket.disableAutomaticReconnection();
        conn.socket.close(args.code, args.reason);
        return 0;
    }

    int connReadyState(lua_State* L) {
        auto& conn = checkOpenConnection(L);
        if (conn.stopped.load()) {
            push(L, "closed");
            return 1;
        }
        push(L, wsReadyStateName(conn.socket.getReadyState()));
        return 1;
    }

    int connUrl(lua_State* L) {
        auto& conn = checkOpenConnection(L);
        push(L, conn.socket.getUrl());
        return 1;
    }

    int connOnOpen(lua_State* L) {
        return setConnCallback(L, &WsConnection::onOpen);
    }

    int connOnMessage(lua_State* L) {
        return setConnCallback(L, &WsConnection::onMessage);
    }

    int connOnClose(lua_State* L) {
        return setConnCallback(L, &WsConnection::onClose);
    }

    int connOnError(lua_State* L) {
        return setConnCallback(L, &WsConnection::onError);
    }
} // namespace luax::wsdetail
