#include "bindings/websocket/WebSocketInternal.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <Geode/Geode.hpp>
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
            geode::queueInMainThread([weak = std::move(weak), data = std::move(data), binary] {
                auto conn = weak.lock();
                if (!conn || conn->stopped.load()) return;
                struct Ctx {
                    std::string const* data;
                    bool binary;
                } ctx{&data, binary};
                invokeWsCallback(
                    conn->slot(&WsConnection::onMessage),
                    "websocket.onMessage",
                    2,
                    +[](lua_State* L, void* raw) {
                        auto* c = static_cast<Ctx*>(raw);
                        push(L, *c->data);
                        push(L, c->binary);
                    },
                    &ctx
                );
            });
        }

        void queueCloseEvent(
            std::weak_ptr<WsConnection> weak, std::uint16_t code, std::string reason, bool remote
        ) {
            geode::queueInMainThread([weak = std::move(weak), code, reason = std::move(reason), remote] {
                auto conn = weak.lock();
                if (!conn || conn->stopped.load()) return;
                struct Ctx {
                    std::uint16_t code;
                    std::string const* reason;
                    bool remote;
                } ctx{code, &reason, remote};
                invokeWsCallback(
                    conn->slot(&WsConnection::onClose),
                    "websocket.onClose",
                    3,
                    +[](lua_State* L, void* raw) {
                        auto* c = static_cast<Ctx*>(raw);
                        push(L, static_cast<int>(c->code));
                        push(L, *c->reason);
                        push(L, c->remote);
                    },
                    &ctx
                );
            });
        }

        void queueErrorEvent(std::weak_ptr<WsConnection> weak, std::string message) {
            geode::queueInMainThread([weak = std::move(weak), message = std::move(message)] {
                auto conn = weak.lock();
                if (!conn || conn->stopped.load()) return;
                struct Ctx {
                    std::string const* message;
                } ctx{&message};
                invokeWsCallback(
                    conn->slot(&WsConnection::onError),
                    "websocket.onError",
                    1,
                    +[](lua_State* L, void* raw) {
                        push(L, *static_cast<Ctx*>(raw)->message);
                    },
                    &ctx
                );
            });
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
            if (data.size() > kMaxWebSocketSendBytes) {
                return pushNilErr(L, kWsSendSizeExceededMsg);
            }
            if (conn.stopped.load()) {
                return pushNilErr(L, kWsConnectionClosedMsg);
            }
            auto info = binary ? conn.socket.sendBinary(data) : conn.socket.sendText(data);
            if (!info.success) {
                return pushNilErr(L, kWsConnectionClosedMsg);
            }
            push(L, true);
            return 1;
        }
    } // namespace

    int wsConnect(lua_State* L) {
        auto url = check<std::string>(L, 1, kMethod);
        if (!url.starts_with("ws://") && !url.starts_with("wss://")) {
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
        auto info = conn.socket.ping(payload);
        push(L, info.success);
        return 1;
    }

    int connClose(lua_State* L) {
        auto& conn = checkOpenConnection(L);
        std::uint16_t code = ix::WebSocketCloseConstants::kNormalClosureCode;
        std::string reason = ix::WebSocketCloseConstants::kNormalClosureMessage;
        if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
            int value = check<int>(L, 2, "close");
            if (value < 1000 || value > 4999) luaL_error(L, "close expected code 1000..4999");
            code = static_cast<std::uint16_t>(value);
        }
        if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
            reason = check<std::string>(L, 3, "close");
        }
        conn.socket.disableAutomaticReconnection();
        conn.socket.close(code, reason);
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
