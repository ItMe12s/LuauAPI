#pragma once

#include "core/Config.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/lifecycle/ShutdownHook.hpp"
#include "framework/lifecycle/WeakHandlePool.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/UserdataTags.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Log.hpp>
#include <atomic>
#include <cstddef>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>
#include <lua.h>
#include <lualib.h>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace luax {
    inline constexpr char const* kWsConnectionMeta = "luax.WebSocketConnection";
    inline constexpr char const* kWsServerMeta = "luax.WebSocketServer";
    inline constexpr char const* kWsPeerMeta = "luax.WebSocketPeer";

    inline constexpr char kWsSendSizeExceededMsg[] = "websocket message exceeds maximum send size";
    inline constexpr char kWsReceiveSizeExceededMsg[] =
        "websocket message exceeds maximum receive size";

    inline bool wsSendWithinLimit(std::size_t size) {
        return size <= kMaxWebSocketSendBytes;
    }

    inline int pushWsSendSizeExceeded(lua_State* L) {
        return pushNilErr(L, kWsSendSizeExceededMsg);
    }

    struct WsCloseArgs {
        std::uint16_t code = ix::WebSocketCloseConstants::kNormalClosureCode;
        std::string reason = ix::WebSocketCloseConstants::kNormalClosureMessage;
    };

    inline WsCloseArgs parseCloseArgs(lua_State* L) {
        WsCloseArgs args;
        if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
            int value = check<int>(L, 2, "close");
            if (value < 1000 || value > 4999) luaL_error(L, "close expected code 1000..4999");
            args.code = static_cast<std::uint16_t>(value);
        }
        if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
            args.reason = check<std::string>(L, 3, "close");
        }
        return args;
    }

    inline int wsSendResult(lua_State* L, ix::WebSocketSendInfo const& info, char const* closedMsg) {
        if (!info.success) {
            return pushNilErr(L, closedMsg);
        }
        push(L, true);
        return 1;
    }

    template <class Owner, class Ctx, void (*PushArgs)(lua_State*, Ctx const*)>
    void queueWsEvent(
        std::weak_ptr<Owner> weak, std::shared_ptr<LuaCallback> Owner::* slotMember,
        char const* context, int nargs, Ctx ctx
    ) {
        geode::queueInMainThread(
            [weak = std::move(weak), slotMember, context, nargs, ctx = std::move(ctx)]() mutable {
                auto owner = weak.lock();
                if (!owner || owner->stopped.load()) return;
                invokeWsCallback(
                    owner->slot(slotMember),
                    context,
                    nargs,
                    +[](lua_State* L, void* raw) {
                        PushArgs(L, static_cast<Ctx*>(raw));
                    },
                    &ctx
                );
            }
        );
    }

    inline constexpr char kWsConnectionClosedMsg[] = "websocket connection is closed";
    inline constexpr char kWsPeerDisconnectedMsg[] = "websocket peer is disconnected";
    inline constexpr char kWsServerStoppedMsg[] = "websocket server is stopped";

    struct WsConnection {
        ix::WebSocket socket;
        std::mutex mutex;
        std::shared_ptr<LuaCallback> onOpen;
        std::shared_ptr<LuaCallback> onMessage;
        std::shared_ptr<LuaCallback> onClose;
        std::shared_ptr<LuaCallback> onError;
        std::atomic<bool> stopped{false};

        ~WsConnection() {
            shutdown();
        }

        void shutdown() {
            if (stopped.exchange(true)) return;
            socket.disableAutomaticReconnection();
            socket.stop();
            std::lock_guard lock(mutex);
            onOpen.reset();
            onMessage.reset();
            onClose.reset();
            onError.reset();
        }

        std::shared_ptr<LuaCallback> slot(std::shared_ptr<LuaCallback> WsConnection::* member) {
            std::lock_guard lock(mutex);
            return this->*member;
        }

        void setSlot(
            std::shared_ptr<LuaCallback> WsConnection::* member, std::shared_ptr<LuaCallback> cb
        ) {
            std::lock_guard lock(mutex);
            this->*member = std::move(cb);
        }
    };

    struct WsPeer {
        std::weak_ptr<ix::WebSocket> socket;
        std::string id;
        std::string remoteIp;
        int remotePort = 0;
        std::atomic<bool> open{true};
    };

    struct WsServer {
        std::unique_ptr<ix::WebSocketServer> server;
        std::mutex mutex;
        std::shared_ptr<LuaCallback> onClientConnect;
        std::shared_ptr<LuaCallback> onMessage;
        std::shared_ptr<LuaCallback> onClientDisconnect;
        std::shared_ptr<LuaCallback> onError;
        std::map<std::string, std::shared_ptr<WsPeer>> peers;
        int port = 0;
        std::atomic<bool> stopped{false};

        ~WsServer() {
            shutdown();
        }

        void shutdown() {
            if (stopped.exchange(true)) return;
            if (server) server->stop();
            std::lock_guard lock(mutex);
            for (auto& [id, peer] : peers) {
                peer->open.store(false);
            }
            peers.clear();
            onClientConnect.reset();
            onMessage.reset();
            onClientDisconnect.reset();
            onError.reset();
        }

        std::shared_ptr<LuaCallback> slot(std::shared_ptr<LuaCallback> WsServer::* member) {
            std::lock_guard lock(mutex);
            return this->*member;
        }

        void setSlot(std::shared_ptr<LuaCallback> WsServer::* member, std::shared_ptr<LuaCallback> cb) {
            std::lock_guard lock(mutex);
            this->*member = std::move(cb);
        }

        std::shared_ptr<WsPeer> findPeer(std::string const& id) {
            std::lock_guard lock(mutex);
            auto it = peers.find(id);
            return it != peers.end() ? it->second : nullptr;
        }
    };

    struct WsConnectionBox {
        std::shared_ptr<WsConnection> conn;
    };

    struct WsServerBox {
        std::shared_ptr<WsServer> server;
    };

    struct WsPeerBox {
        std::shared_ptr<WsPeer> peer;
    };

    inline WeakHandlePool<WsConnection>& activeWsConnections() {
        static WeakHandlePool<WsConnection> connections;
        return connections;
    }

    inline WeakHandlePool<WsServer>& activeWsServers() {
        static WeakHandlePool<WsServer> servers;
        return servers;
    }

    inline bool& wsShutdownHookRegistered() {
        static bool registered = false;
        return registered;
    }

    inline void clearWsState() {
        activeWsConnections().clearAll([](WsConnection& conn) {
            conn.shutdown();
        });
        activeWsServers().clearAll([](WsServer& server) {
            server.shutdown();
        });
        wsShutdownHookRegistered() = false;
    }

    inline void ensureWsShutdownHook() {
        ensureShutdownHook(wsShutdownHookRegistered(), &clearWsState);
    }

    inline void ensureWsNetSystem() {
        static bool initialized = []() {
            return ix::initNetSystem();
        }();
        (void)initialized;
    }

    inline void invokeWsCallback(
        std::shared_ptr<LuaCallback> const& cb, char const* context, int nargs,
        LuaCallback::PushArgsFn pushArgs, void* pushCtx
    ) {
        if (!cb || !cb->valid()) return;
        if (!cb->invoke(nargs, 0, context, kHookScriptDeadlineMs, pushArgs, pushCtx)) {
            geode::log::warn("[lua:{}] callback failed", context);
        }
    }

    inline char const* wsReadyStateName(ix::ReadyState state) {
        switch (state) {
            case ix::ReadyState::Connecting: return "connecting";
            case ix::ReadyState::Open: return "open";
            case ix::ReadyState::Closing: return "closing";
            default: return "closed";
        }
    }

    inline WsConnectionBox* checkWsConnection(lua_State* L, int idx) {
        return static_cast<WsConnectionBox*>(luaL_checkudata(L, idx, kWsConnectionMeta));
    }

    inline WsServerBox* checkWsServer(lua_State* L, int idx) {
        return static_cast<WsServerBox*>(luaL_checkudata(L, idx, kWsServerMeta));
    }

    inline WsPeerBox* checkWsPeer(lua_State* L, int idx) {
        return static_cast<WsPeerBox*>(luaL_checkudata(L, idx, kWsPeerMeta));
    }

    inline void pushWsConnection(lua_State* L, std::shared_ptr<WsConnection> conn) {
        auto* box = static_cast<WsConnectionBox*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(WsConnectionBox), detail::wsConnectionTag())
        );
        new (box) WsConnectionBox{std::move(conn)};
    }

    inline void pushWsServer(lua_State* L, std::shared_ptr<WsServer> server) {
        auto* box = static_cast<WsServerBox*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(WsServerBox), detail::wsServerTag())
        );
        new (box) WsServerBox{std::move(server)};
    }

    inline void pushWsPeer(lua_State* L, std::shared_ptr<WsPeer> peer) {
        auto* box = static_cast<WsPeerBox*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(WsPeerBox), detail::wsPeerTag())
        );
        new (box) WsPeerBox{std::move(peer)};
    }

    inline void wsConnectionDtor(lua_State*, void* ud) {
        auto* box = static_cast<WsConnectionBox*>(ud);
        if (box->conn) box->conn->shutdown();
        box->~WsConnectionBox();
    }

    inline void wsServerDtor(lua_State*, void* ud) {
        auto* box = static_cast<WsServerBox*>(ud);
        if (box->server) box->server->shutdown();
        box->~WsServerBox();
    }

    inline void wsPeerDtor(lua_State*, void* ud) {
        static_cast<WsPeerBox*>(ud)->~WsPeerBox();
    }
} // namespace luax

namespace luax::wsdetail {
    int wsConnect(lua_State* L);

    int connSend(lua_State* L);
    int connSendBinary(lua_State* L);
    int connPing(lua_State* L);
    int connClose(lua_State* L);
    int connReadyState(lua_State* L);
    int connUrl(lua_State* L);
    int connOnOpen(lua_State* L);
    int connOnMessage(lua_State* L);
    int connOnClose(lua_State* L);
    int connOnError(lua_State* L);

    int wsServe(lua_State* L);

    int serverBroadcast(lua_State* L);
    int serverBroadcastBinary(lua_State* L);
    int serverClients(lua_State* L);
    int serverPort(lua_State* L);
    int serverStop(lua_State* L);
    int serverOnClientConnect(lua_State* L);
    int serverOnMessage(lua_State* L);
    int serverOnClientDisconnect(lua_State* L);
    int serverOnError(lua_State* L);

    int peerSend(lua_State* L);
    int peerSendBinary(lua_State* L);
    int peerClose(lua_State* L);
    int peerRemoteAddress(lua_State* L);
    int peerId(lua_State* L);

    void registerMetatables(lua_State* L);
} // namespace luax::wsdetail
