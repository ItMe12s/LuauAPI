#include "bindings/websocket/WebSocketInternal.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/TableUtil.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>

namespace luax::wsdetail {
    using namespace luax;

    namespace {
        void registerMethods(
            lua_State* L, char const* meta, int tag, luaL_Reg const* methods, lua_Destructor dtor
        ) {
            if (luaL_newmetatable(L, meta)) {
                for (luaL_Reg const* reg = methods; reg->name != nullptr; ++reg) {
                    setTableCFunction(L, -1, reg->name, reg->func);
                }
                lua_pushvalue(L, -1);
                lua_setfield(L, -2, "__index");
                lua_pushstring(L, "locked");
                lua_setfield(L, -2, "__metatable");
            }
            lua_pop(L, 1);

            lua_getuserdatametatable(L, tag);
            if (!lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return;
            }
            lua_pop(L, 1);

            luaL_getmetatable(L, meta);
            lua_setuserdatametatable(L, tag);
            lua_setuserdatadtor(L, tag, dtor);
        }
    } // namespace

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
        registerMethods(
            L, kWsConnectionMeta, detail::wsConnectionTag(), connectionMethods, &wsConnectionDtor
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
        registerMethods(L, kWsServerMeta, detail::wsServerTag(), serverMethods, &wsServerDtor);

        luaL_Reg peerMethods[] = {
            {"send", peerSend},
            {"sendBinary", peerSendBinary},
            {"close", peerClose},
            {"remoteAddress", peerRemoteAddress},
            {"id", peerId},
            {nullptr, nullptr},
        };
        registerMethods(L, kWsPeerMeta, detail::wsPeerTag(), peerMethods, &wsPeerDtor);
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
