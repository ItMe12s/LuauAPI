#include "bindings/websocket/WebSocketInternal.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>

namespace luax::wsdetail {
    using namespace luax;

    void registerMetatables(lua_State* L) {
        luaL_Reg connectionMethods[] = {
            {"send", connSend},
            {"sendBinary", connSendBinary},
            {"ping", connPing},
            {"close", connClose},
            {"readyState", connReadyState},
            {"url", connUrl},
            {"onOpen", connOnOpen},
            {"onMessage", connOnMessage},
            {"onClose", connOnClose},
            {"onError", connOnError},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(
            L, kWsConnectionMeta, detail::wsConnectionTag(), connectionMethods, std::nullopt, &wsConnectionDtor
        );

        luaL_Reg serverMethods[] = {
            {"broadcast", serverBroadcast},
            {"broadcastBinary", serverBroadcastBinary},
            {"clients", serverClients},
            {"port", serverPort},
            {"stop", serverStop},
            {"onClientConnect", serverOnClientConnect},
            {"onMessage", serverOnMessage},
            {"onClientDisconnect", serverOnClientDisconnect},
            {"onError", serverOnError},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(
            L, kWsServerMeta, detail::wsServerTag(), serverMethods, std::nullopt, &wsServerDtor
        );

        luaL_Reg peerMethods[] = {
            {"send", peerSend},
            {"sendBinary", peerSendBinary},
            {"close", peerClose},
            {"remoteAddress", peerRemoteAddress},
            {"id", peerId},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(
            L, kWsPeerMeta, detail::wsPeerTag(), peerMethods, std::nullopt, &wsPeerDtor
        );
    }
} // namespace luax::wsdetail

namespace luax {
    geode::Result<void> registerWebSocket(lua_State* L) {
        wsdetail::registerMetatables(L);

        lua_newtable(L);
        setTableCFunction(L, -1, "connect", &wsdetail::wsConnect);
        setTableCFunction(L, -1, "serve", &wsdetail::wsServe);
        lua_setglobal(L, "websocket");
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(websocket_lib, luax::registerWebSocket)
#endif
