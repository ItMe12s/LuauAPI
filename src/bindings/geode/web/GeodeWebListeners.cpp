#include "bindings/geode/web/GeodeWebConstants.manifest.hpp"
#include "bindings/geode/web/GeodeWebInternal.hpp"
#include "bindings/geode/web/WebCaps.hpp"
#include "core/Runtime.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <Geode/loader/Priority.hpp>
#include <Geode/utils/web.hpp>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace luax::webdetail {
    using namespace luax;

    namespace {
        bool invokeRequestEvent(
            std::shared_ptr<LuaCallback> const& cb, char const* context,
            std::optional<std::string_view> modID, web::WebRequest& request
        ) {
            if (!cb || !cb->valid()) return false;

            if (!Runtime::isMainThread()) {
                static bool loggedOffThreadBlock = false;
                if (!loggedOffThreadBlock) {
                    loggedOffThreadBlock = true;
                    geode::log::warn("[lua:{}] off-thread intercept blocked", context);
                }
                return true;
            }

            struct Ctx {
                std::optional<std::string_view> modID;
                web::WebRequest* request;
                bool stop = false;
            } ctx{modID, &request, false};

            bool ok = cb->invoke(
                modID ? 2 : 1,
                1,
                context,
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    if (c->modID) push(L, *c->modID);
                    pushRequest(L, *c->request);
                },
                &ctx,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    c->stop = lua_toboolean(L, -1) != 0;
                },
                &ctx
            );
            if (!ok) {
                logCallbackFailure(context);
                return true;
            }
            return ctx.stop;
        }

        bool invokeResponseEventNow(
            std::shared_ptr<LuaCallback> const& cb, char const* context,
            std::optional<std::string_view> modID, web::WebResponse const& response
        ) {
            if (!cb || !cb->valid()) return false;

            struct Ctx {
                std::optional<std::string_view> modID;
                web::WebResponse response;
                bool stop = false;
            } ctx{modID, response, false};

            int callbackArgs = modID ? 3 : 2;
            bool ok = cb->invoke(
                callbackArgs,
                1,
                context,
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    if (c->modID) push(L, *c->modID);
                    if (!responseDataWithinLimit(c->response.data().size())) {
                        pushNilErr(L, kWebResponseSizeExceededMsg);
                    }
                    else {
                        pushResponse(L, std::move(c->response));
                        lua_pushnil(L);
                    }
                },
                &ctx,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    c->stop = lua_toboolean(L, -1) != 0;
                },
                &ctx
            );
            if (!ok) {
                logCallbackFailure(context);
            }
            return ok && ctx.stop;
        }

        bool invokeResponseEvent(
            std::shared_ptr<LuaCallback> cb, char const* context,
            std::optional<std::string_view> modID, web::WebResponse const& response
        ) {
            if (!cb || !cb->valid()) return false;
            if (Runtime::isMainThread()) {
                return invokeResponseEventNow(cb, context, modID, response);
            }
            // Off-thread response listeners run later for side effects only,
            // they cannot stop propagation synchronously. Trust me on this one.
            std::optional<std::string> ownedModID;
            if (modID) ownedModID = std::string(*modID);
            geode::queueInMainThread(
                [cb = std::move(cb), context, ownedModID = std::move(ownedModID), response]() mutable {
                    std::optional<std::string_view> view;
                    if (ownedModID) view = std::string_view(*ownedModID);
                    (void)invokeResponseEventNow(cb, context, view, response);
                }
            );
            return false;
        }

        void rememberListener(std::shared_ptr<WebListenerState> const& state) {
            activeListeners().track(state);
            compactWeakState();
            ensureShutdownHook();
        }

        int registerWebListener(
            lua_State* L, int callbackIdx, int priorityIdx,
            std::function<geode::ListenerHandle(std::shared_ptr<LuaCallback>, int)> connect
        ) {
            luaL_checktype(L, callbackIdx, LUA_TFUNCTION);
            auto cb = std::make_shared<LuaCallback>(L, callbackIdx);
            auto state = std::make_shared<WebListenerState>();
            state->handle = connect(cb, optPriority(L, priorityIdx));
            rememberListener(state);
            pushListener(L, std::move(state));
            return 1;
        }

        enum class ListenerFilterKind : std::uint8_t {
            Global,
            ModId,
            ById,
        };

        enum class ListenerEventKind : std::uint8_t {
            RequestIntercept,
            Response,
        };

        struct ListenerDescriptor {
            char const* context;
            ListenerFilterKind filter;
            ListenerEventKind event;
        };

        geode::ListenerHandle connectRequestIntercept(
            std::shared_ptr<LuaCallback> cb, int priority, ListenerDescriptor const& desc,
            std::optional<std::string> modID, std::optional<std::size_t> id
        ) {
            char const* ctx = desc.context;
            switch (desc.filter) {
                case ListenerFilterKind::Global:
                    return web::WebRequestInterceptEvent().listen(
                        [cb, ctx](std::string_view modIDView, web::WebRequest& req) {
                            return invokeRequestEvent(cb, ctx, modIDView, req);
                        },
                        priority
                    );
                case ListenerFilterKind::ModId:
                    return web::WebRequestInterceptEvent(std::move(*modID))
                        .listen(
                            [cb, ctx](web::WebRequest& req) {
                                return invokeRequestEvent(cb, ctx, std::nullopt, req);
                            },
                            priority
                        );
                case ListenerFilterKind::ById:
                    return web::IDBasedWebRequestInterceptEvent(*id).listen(
                        [cb, ctx](web::WebRequest& req) {
                            return invokeRequestEvent(cb, ctx, std::nullopt, req);
                        },
                        priority
                    );
                default: std::unreachable();
            }
        }

        geode::ListenerHandle connectResponse(
            std::shared_ptr<LuaCallback> cb, int priority, ListenerDescriptor const& desc,
            std::optional<std::string> modID, std::optional<std::size_t> id
        ) {
            char const* ctx = desc.context;
            switch (desc.filter) {
                case ListenerFilterKind::Global:
                    return web::WebResponseEvent().listen(
                        [cb, ctx](std::string_view modIDView, web::WebResponse const& response) {
                            return invokeResponseEvent(cb, ctx, modIDView, response);
                        },
                        priority
                    );
                case ListenerFilterKind::ModId:
                    return web::WebResponseEvent(std::move(*modID))
                        .listen(
                            [cb, ctx](web::WebResponse const& response) {
                                return invokeResponseEvent(cb, ctx, std::nullopt, response);
                            },
                            priority
                        );
                case ListenerFilterKind::ById:
                    return web::IDBasedWebResponseEvent(*id).listen(
                        [cb, ctx](web::WebResponse const& response) {
                            return invokeResponseEvent(cb, ctx, std::nullopt, response);
                        },
                        priority
                    );
                default: std::unreachable();
            }
        }

        ListenerDescriptor const kListenerDescriptors[] = {
            {"geode.utils.web.onRequestIntercept",
             ListenerFilterKind::Global,
             ListenerEventKind::RequestIntercept},
            {"geode.utils.web.onRequestInterceptFor",
             ListenerFilterKind::ModId,
             ListenerEventKind::RequestIntercept},
            {"geode.utils.web.onRequestInterceptById",
             ListenerFilterKind::ById,
             ListenerEventKind::RequestIntercept},
            {"geode.utils.web.onResponse", ListenerFilterKind::Global, ListenerEventKind::Response},
            {"geode.utils.web.onResponseFor", ListenerFilterKind::ModId, ListenerEventKind::Response},
            {"geode.utils.web.onResponseById", ListenerFilterKind::ById, ListenerEventKind::Response},
        };

        enum class ListenerId : std::size_t {
            RequestIntercept = 0,
            RequestInterceptFor,
            RequestInterceptById,
            Response,
            ResponseFor,
            ResponseById,
        };

        int registerListenerFromDescriptor(lua_State* L, ListenerDescriptor const& desc) {
            int callbackIdx = 0;
            int priorityIdx = 0;
            std::optional<std::string> modID;
            std::optional<std::size_t> id;

            switch (desc.filter) {
                case ListenerFilterKind::Global:
                    callbackIdx = 1;
                    priorityIdx = 2;
                    break;
                case ListenerFilterKind::ModId:
                    modID = check<std::string>(L, 1, desc.context);
                    callbackIdx = 2;
                    priorityIdx = 3;
                    break;
                case ListenerFilterKind::ById:
                    id = static_cast<std::size_t>(checkNonNegativeInteger(L, 1, desc.context));
                    callbackIdx = 2;
                    priorityIdx = 3;
                    break;
            }

            auto connect = [desc, modID = std::move(modID), id](
                               std::shared_ptr<LuaCallback> cb, int priority
                           ) mutable -> geode::ListenerHandle {
                if (desc.event == ListenerEventKind::RequestIntercept) {
                    return connectRequestIntercept(cb, priority, desc, std::move(modID), id);
                }
                return connectResponse(cb, priority, desc, std::move(modID), id);
            };

            return registerWebListener(L, callbackIdx, priorityIdx, std::move(connect));
        }

        int listenerDispatch(lua_State* L, ListenerId id) {
            return registerListenerFromDescriptor(
                L, kListenerDescriptors[static_cast<std::size_t>(id)]
            );
        }

        struct LongEnumEntry {
            char const* name;
            long value;
        };

        constexpr IntEnumEntry kHttpVersionEntries[] = {LUAX_WEB_HTTP_VERSION(LUAX_WEB_INT_ENUM_ENTRY)};

        constexpr IntEnumEntry kProxyTypeEntries[] = {LUAX_WEB_PROXY_TYPE(LUAX_WEB_INT_ENUM_ENTRY)};

        constexpr IntEnumEntry kGeodeWebErrorEntries[] = {
            LUAX_WEB_GEODE_WEB_ERROR(LUAX_WEB_INT_ENUM_ENTRY)
        };

        constexpr LongEnumEntry kHttpAuthEntries[] = {LUAX_WEB_HTTP_AUTH(LUAX_WEB_LONG_ENUM_ENTRY)};

        template <typename T, std::size_t N>
        void registerLongEnumTable(lua_State* L, T const (&entries)[N], char const* tableName) {
            lua_createtable(L, 0, static_cast<int>(N));
            for (auto const& entry : entries) {
                setLongField(L, entry.name, entry.value);
            }
            lua_setfield(L, -2, tableName);
        }
    } // namespace

    int handleCancel(lua_State* L) {
        auto* box = checkHandle(L, 1, "WebHandle:cancel");
        bool didCancel = box->task && !box->task->done;
        if (box->task) box->task->cancel();
        push(L, didCancel);
        return 1;
    }

    int handleId(lua_State* L) {
        auto* box = checkHandle(L, 1, "WebHandle:id");
        if (!box->task) {
            lua_pushnil(L);
            return 1;
        }
        push(L, static_cast<double>(box->task->id));
        return 1;
    }

    int listenerDisconnect(lua_State* L) {
        auto* box = checkListener(L, 1, "WebListenerHandle:disconnect");
        if (box->state) box->state->disconnect();
        return 0;
    }

    int webOnRequestIntercept(lua_State* L) {
        return listenerDispatch(L, ListenerId::RequestIntercept);
    }

    int webOnRequestInterceptFor(lua_State* L) {
        return listenerDispatch(L, ListenerId::RequestInterceptFor);
    }

    int webOnRequestInterceptById(lua_State* L) {
        return listenerDispatch(L, ListenerId::RequestInterceptById);
    }

    int webOnResponse(lua_State* L) {
        return listenerDispatch(L, ListenerId::Response);
    }

    int webOnResponseFor(lua_State* L) {
        return listenerDispatch(L, ListenerId::ResponseFor);
    }

    int webOnResponseById(lua_State* L) {
        return listenerDispatch(L, ListenerId::ResponseById);
    }

    void registerConstants(lua_State* L) {
        registerIntEnumTable(L, kHttpVersionEntries, "HttpVersion");
        registerIntEnumTable(L, kProxyTypeEntries, "ProxyType");
        registerIntEnumTable(L, kGeodeWebErrorEntries, "Error");
        registerLongEnumTable(L, kHttpAuthEntries, "HttpAuth");
    }
} // namespace luax::webdetail
