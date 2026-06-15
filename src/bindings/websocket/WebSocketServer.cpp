#include "bindings/websocket/WebSocketInternal.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/utils/string.hpp>
#include <array>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace luax::wsdetail {
    using namespace luax;

    namespace {
        constexpr char const* kMethod = "websocket.serve";

        struct ClientConnectCtx {
            std::shared_ptr<WsPeer> peer;
            std::map<std::string, std::string> headers;
        };

        struct ClientMessageCtx {
            std::shared_ptr<WsPeer> peer;
            std::string data;
            bool binary;
        };

        struct ClientDisconnectCtx {
            std::shared_ptr<WsPeer> peer;
            std::uint16_t code;
            std::string reason;
        };

        void pushErrorArgs(lua_State* L, std::string const* ctx) {
            push(L, *ctx);
        }

        void pushClientConnectArgs(lua_State* L, ClientConnectCtx const* ctx) {
            pushWsPeer(L, ctx->peer);
            lua_createtable(L, 0, static_cast<int>(ctx->headers.size()));
            for (auto const& [name, value] : ctx->headers) {
                push(L, value);
                lua_setfield(L, -2, name.c_str());
            }
        }

        void pushClientMessageArgs(lua_State* L, ClientMessageCtx const* ctx) {
            pushWsPeer(L, ctx->peer);
            push(L, ctx->data);
            push(L, ctx->binary);
        }

        void pushClientDisconnectArgs(lua_State* L, ClientDisconnectCtx const* ctx) {
            pushWsPeer(L, ctx->peer);
            push(L, static_cast<int>(ctx->code));
            push(L, ctx->reason);
        }

        void queueClientConnectEvent(
            std::weak_ptr<WsServer> weak, std::shared_ptr<WsPeer> peer,
            std::map<std::string, std::string> headers
        ) {
            queueWsEvent<WsServer, ClientConnectCtx, pushClientConnectArgs>(
                std::move(weak),
                &WsServer::onClientConnect,
                "websocket.onClientConnect",
                2,
                ClientConnectCtx{std::move(peer), std::move(headers)}
            );
        }

        void queueClientMessageEvent(
            std::weak_ptr<WsServer> weak, std::shared_ptr<WsPeer> peer, std::string data, bool binary
        ) {
            queueWsEvent<WsServer, ClientMessageCtx, pushClientMessageArgs>(
                std::move(weak),
                &WsServer::onMessage,
                "websocket.server.onMessage",
                3,
                ClientMessageCtx{std::move(peer), std::move(data), binary}
            );
        }

        void queueClientDisconnectEvent(
            std::weak_ptr<WsServer> weak, std::shared_ptr<WsPeer> peer, std::uint16_t code,
            std::string reason
        ) {
            queueWsEvent<WsServer, ClientDisconnectCtx, pushClientDisconnectArgs>(
                std::move(weak),
                &WsServer::onClientDisconnect,
                "websocket.onClientDisconnect",
                3,
                ClientDisconnectCtx{std::move(peer), code, std::move(reason)}
            );
        }

        void queueServerErrorEvent(std::weak_ptr<WsServer> weak, std::string message) {
            queueWsEvent<WsServer, std::string, pushErrorArgs>(
                std::move(weak), &WsServer::onError, "websocket.server.onError", 1, std::move(message)
            );
        }

        void handleServerClientMessage(
            std::weak_ptr<WsServer> const& weak, std::shared_ptr<ix::ConnectionState> const& state,
            ix::WebSocket& socket, ix::WebSocketMessagePtr const& msg
        ) {
            auto srv = weak.lock();
            if (!srv || srv->stopped.load()) return;
            auto peer = srv->findPeer(state->getId());
            if (!peer) return;
            switch (msg->type) {
                case ix::WebSocketMessageType::Open: {
                    std::map<std::string, std::string> headers(
                        msg->openInfo.headers.begin(), msg->openInfo.headers.end()
                    );
                    queueClientConnectEvent(weak, std::move(peer), std::move(headers));
                    break;
                }
                case ix::WebSocketMessageType::Message:
                    if (msg->str.size() > kMaxWebSocketReceiveBytes) {
                        queueServerErrorEvent(
                            weak, "peer " + peer->id + ": " + kWsReceiveSizeExceededMsg
                        );
                        socket.close(1009, "message too big");
                        break;
                    }
                    queueClientMessageEvent(weak, std::move(peer), msg->str, msg->binary);
                    break;
                case ix::WebSocketMessageType::Close: {
                    peer->open.store(false);
                    {
                        std::lock_guard lock(srv->mutex);
                        srv->peers.erase(peer->id);
                    }
                    queueClientDisconnectEvent(
                        weak, std::move(peer), msg->closeInfo.code, msg->closeInfo.reason
                    );
                    break;
                }
                case ix::WebSocketMessageType::Error:
                    queueServerErrorEvent(weak, "peer " + peer->id + ": " + msg->errorInfo.reason);
                    break;
                default: break;
            }
        }

        void installServerCallbacks(std::shared_ptr<WsServer> const& srv) {
            std::weak_ptr<WsServer> weak = srv;

            srv->server->setOnConnectionCallback(
                [weak](
                    std::weak_ptr<ix::WebSocket> socketWeak, std::shared_ptr<ix::ConnectionState> state
                ) {
                    auto srv = weak.lock();
                    if (!srv || srv->stopped.load()) return;

                    auto peer = std::make_shared<WsPeer>();
                    peer->socket = socketWeak;
                    peer->id = state->getId();
                    peer->remoteIp = state->getRemoteIp();
                    peer->remotePort = state->getRemotePort();
                    {
                        std::lock_guard lock(srv->mutex);
                        srv->peers[peer->id] = peer;
                    }

                    auto socket = socketWeak.lock();
                    if (!socket) return;
                    ix::WebSocket* socketRaw = socket.get();
                    socket->setOnMessageCallback([weak,
                                                  state = std::move(state),
                                                  socketRaw](ix::WebSocketMessagePtr const& msg) {
                        handleServerClientMessage(weak, state, *socketRaw, msg);
                    });
                }
            );
        }

        WsServer& checkRunningServer(lua_State* L) {
            auto* box = checkWsServer(L, 1);
            if (!box->server) luaL_error(L, "%s", kWsServerStoppedMsg);
            return *box->server;
        }

        int setServerCallback(lua_State* L, std::shared_ptr<LuaCallback> WsServer::* member) {
            auto& srv = checkRunningServer(L);
            luaL_checktype(L, 2, LUA_TFUNCTION);
            srv.setSlot(member, std::make_shared<LuaCallback>(L, 2));
            lua_pushvalue(L, 1);
            return 1;
        }

        int broadcastData(lua_State* L, bool binary) {
            auto& srv = checkRunningServer(L);
            auto data = check<std::string>(L, 2, binary ? "broadcastBinary" : "broadcast");
            if (!wsSendWithinLimit(data.size())) {
                return pushWsSendSizeExceeded(L);
            }
            if (srv.stopped.load()) {
                return pushNilErr(L, kWsServerStoppedMsg);
            }
            for (auto const& client : srv.server->getClients()) {
                if (client->getReadyState() != ix::ReadyState::Open) continue;
                if (binary) {
                    client->sendBinary(data);
                }
                else {
                    client->sendText(data);
                }
            }
            push(L, true);
            return 1;
        }

        ix::WebSocket* lockOpenPeer(WsPeerBox* box, std::shared_ptr<ix::WebSocket>& keepAlive) {
            if (!box->peer || !box->peer->open.load()) return nullptr;
            keepAlive = box->peer->socket.lock();
            return keepAlive.get();
        }

        int peerSendData(lua_State* L, bool binary) {
            auto* box = checkWsPeer(L, 1);
            auto data = check<std::string>(L, 2, binary ? "sendBinary" : "send");
            if (!wsSendWithinLimit(data.size())) {
                return pushWsSendSizeExceeded(L);
            }
            std::shared_ptr<ix::WebSocket> keepAlive;
            auto* socket = lockOpenPeer(box, keepAlive);
            if (!socket) return pushNilErr(L, kWsPeerDisconnectedMsg);
            auto info = binary ? socket->sendBinary(data) : socket->sendText(data);
            return wsSendResult(L, info, kWsPeerDisconnectedMsg);
        }
    } // namespace

    int wsServe(lua_State* L) {
        int port = check<int>(L, 1, kMethod);
        if (port < 1 || port > 65535) {
            return pushNilErr(L, "websocket.serve expected port 1..65535");
        }

        std::string host = "127.0.0.1";
        if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
            luaL_checktype(L, 2, LUA_TTABLE);
            if (auto value = optStringField(L, 2, "host", kMethod)) {
                host = std::move(*value);
            }
        }

        if (activeWsServers().compactAndCountLive() >= kMaxWebSocketServers) {
            return pushNilErr(L, "too many websocket servers");
        }

        ensureWsNetSystem();
        auto srv = std::make_shared<WsServer>();
        srv->port = port;
        srv->server = std::make_unique<ix::WebSocketServer>(
            port, host, ix::SocketServer::kDefaultTcpBacklog, kMaxWebSocketServerClients
        );
        srv->server->disablePerMessageDeflate();
        installServerCallbacks(srv);

        auto listenResult = srv->server->listen();
        if (!listenResult.first) {
            return pushNilErr(L, listenResult.second);
        }
        srv->server->start();

        activeWsServers().track(srv);
        ensureWsShutdownHook();

        pushWsServer(L, std::move(srv));
        return 1;
    }

    int serverBroadcast(lua_State* L) {
        return broadcastData(L, false);
    }

    int serverBroadcastBinary(lua_State* L) {
        return broadcastData(L, true);
    }

    int serverClients(lua_State* L) {
        auto& srv = checkRunningServer(L);
        std::lock_guard lock(srv.mutex);
        lua_createtable(L, static_cast<int>(srv.peers.size()), 0);
        int index = 1;
        for (auto const& [id, peer] : srv.peers) {
            pushWsPeer(L, peer);
            lua_rawseti(L, -2, index++);
        }
        return 1;
    }

    int serverPort(lua_State* L) {
        auto& srv = checkRunningServer(L);
        push(L, srv.port);
        return 1;
    }

    int serverStop(lua_State* L) {
        auto& srv = checkRunningServer(L);
        srv.shutdown();
        return 0;
    }

    int serverOnClientConnect(lua_State* L) {
        return setServerCallback(L, &WsServer::onClientConnect);
    }

    int serverOnMessage(lua_State* L) {
        return setServerCallback(L, &WsServer::onMessage);
    }

    int serverOnClientDisconnect(lua_State* L) {
        return setServerCallback(L, &WsServer::onClientDisconnect);
    }

    int serverOnError(lua_State* L) {
        return setServerCallback(L, &WsServer::onError);
    }

    int peerSend(lua_State* L) {
        return peerSendData(L, false);
    }

    int peerSendBinary(lua_State* L) {
        return peerSendData(L, true);
    }

    int peerClose(lua_State* L) {
        auto* box = checkWsPeer(L, 1);
        auto args = parseCloseArgs(L);
        std::shared_ptr<ix::WebSocket> keepAlive;
        auto* socket = lockOpenPeer(box, keepAlive);
        if (socket) socket->close(args.code, args.reason);
        return 0;
    }

    int peerRemoteAddress(lua_State* L) {
        auto* box = checkWsPeer(L, 1);
        if (!box->peer) {
            lua_pushnil(L);
            return 1;
        }
        push(
            L,
            geode::utils::string::join(
                std::array{box->peer->remoteIp, geode::utils::numToString(box->peer->remotePort)},
                ":"
            )
        );
        return 1;
    }

    int peerId(lua_State* L) {
        auto* box = checkWsPeer(L, 1);
        if (!box->peer) {
            lua_pushnil(L);
            return 1;
        }
        push(L, box->peer->id);
        return 1;
    }
} // namespace luax::wsdetail
