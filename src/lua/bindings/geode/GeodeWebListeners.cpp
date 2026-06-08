#include "lua/bindings/framework/LuaCallback.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/GeodeWebInternal.hpp"
#include "lua/bindings/geode/WebCaps.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/loader/Priority.hpp>
#include <Geode/utils/web.hpp>
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
            if (!Runtime::isMainThread()) {
                // Must run on the main thread so Lua can change the request
                // and return stop before Geode continues.
                static bool loggedOffThreadSkip = false;
                if (!loggedOffThreadSkip) {
                    loggedOffThreadSkip = true;
                    geode::log::warn("[lua:{}] off-thread intercept skipped", context);
                }
                return false;
            }
            if (!cb || !cb->valid()) return false;

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
                logWebCallbackFailure(context);
            }
            return ok && ctx.stop;
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
                        lua_pushnil(L);
                        push(L, std::string(kWebResponseSizeExceededMsg));
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
                logWebCallbackFailure(context);
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
            // This is running outside the main thread.
            // The listener will be executed on the main thread later.
            // Only use these response hooks for side effects.
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
            activeListeners().push_back(state);
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
        return registerWebListener(L, 1, 2, [](std::shared_ptr<LuaCallback> cb, int priority) {
            return web::WebRequestInterceptEvent().listen(
                [cb](std::string_view modID, web::WebRequest& req) {
                    return invokeRequestEvent(cb, "geode.utils.web.onRequestIntercept", modID, req);
                },
                priority
            );
        });
    }

    int webOnRequestInterceptFor(lua_State* L) {
        auto modID = check<std::string>(L, 1, "geode.utils.web.onRequestInterceptFor");
        return registerWebListener(
            L, 2, 3, [modID = std::move(modID)](std::shared_ptr<LuaCallback> cb, int priority) {
                return web::WebRequestInterceptEvent(modID).listen(
                    [cb](web::WebRequest& req) {
                        return invokeRequestEvent(
                            cb, "geode.utils.web.onRequestInterceptFor", std::nullopt, req
                        );
                    },
                    priority
                );
            }
        );
    }

    int webOnRequestInterceptById(lua_State* L) {
        auto id = static_cast<std::size_t>(
            checkNonNegativeInteger(L, 1, "geode.utils.web.onRequestInterceptById")
        );
        return registerWebListener(L, 2, 3, [id](std::shared_ptr<LuaCallback> cb, int priority) {
            return web::IDBasedWebRequestInterceptEvent(id).listen(
                [cb](web::WebRequest& req) {
                    return invokeRequestEvent(
                        cb, "geode.utils.web.onRequestInterceptById", std::nullopt, req
                    );
                },
                priority
            );
        });
    }

    int webOnResponse(lua_State* L) {
        return registerWebListener(L, 1, 2, [](std::shared_ptr<LuaCallback> cb, int priority) {
            return web::WebResponseEvent().listen(
                [cb](std::string_view modID, web::WebResponse const& response) {
                    return invokeResponseEvent(cb, "geode.utils.web.onResponse", modID, response);
                },
                priority
            );
        });
    }

    int webOnResponseFor(lua_State* L) {
        auto modID = check<std::string>(L, 1, "geode.utils.web.onResponseFor");
        return registerWebListener(
            L, 2, 3, [modID = std::move(modID)](std::shared_ptr<LuaCallback> cb, int priority) {
                return web::WebResponseEvent(modID).listen(
                    [cb](web::WebResponse const& response) {
                        return invokeResponseEvent(
                            cb, "geode.utils.web.onResponseFor", std::nullopt, response
                        );
                    },
                    priority
                );
            }
        );
    }

    int webOnResponseById(lua_State* L) {
        auto id =
            static_cast<std::size_t>(checkNonNegativeInteger(L, 1, "geode.utils.web.onResponseById"));
        return registerWebListener(L, 2, 3, [id](std::shared_ptr<LuaCallback> cb, int priority) {
            return web::IDBasedWebResponseEvent(id).listen(
                [cb](web::WebResponse const& response) {
                    return invokeResponseEvent(
                        cb, "geode.utils.web.onResponseById", std::nullopt, response
                    );
                },
                priority
            );
        });
    }

    void registerConstants(lua_State* L) {
        lua_createtable(L, 0, 8);
        setIntField(L, "DEFAULT", static_cast<int>(web::HttpVersion::DEFAULT));
        setIntField(L, "VERSION_1_0", static_cast<int>(web::HttpVersion::VERSION_1_0));
        setIntField(L, "VERSION_1_1", static_cast<int>(web::HttpVersion::VERSION_1_1));
        setIntField(L, "VERSION_2_0", static_cast<int>(web::HttpVersion::VERSION_2_0));
        setIntField(L, "VERSION_2TLS", static_cast<int>(web::HttpVersion::VERSION_2TLS));
        setIntField(
            L, "VERSION_2_PRIOR_KNOWLEDGE", static_cast<int>(web::HttpVersion::VERSION_2_PRIOR_KNOWLEDGE)
        );
        setIntField(L, "VERSION_3", static_cast<int>(web::HttpVersion::VERSION_3));
        setIntField(L, "VERSION_3ONLY", static_cast<int>(web::HttpVersion::VERSION_3ONLY));
        lua_setfield(L, -2, "HttpVersion");

        lua_createtable(L, 0, 7);
        setIntField(L, "HTTP", static_cast<int>(web::ProxyType::HTTP));
        setIntField(L, "HTTPS", static_cast<int>(web::ProxyType::HTTPS));
        setIntField(L, "HTTPS2", static_cast<int>(web::ProxyType::HTTPS2));
        setIntField(L, "SOCKS4", static_cast<int>(web::ProxyType::SOCKS4));
        setIntField(L, "SOCKS4A", static_cast<int>(web::ProxyType::SOCKS4A));
        setIntField(L, "SOCKS5", static_cast<int>(web::ProxyType::SOCKS5));
        setIntField(L, "SOCKS5H", static_cast<int>(web::ProxyType::SOCKS5H));
        lua_setfield(L, -2, "ProxyType");

        lua_createtable(L, 0, 4);
        setIntField(
            L,
            "CURL_INITIALIZATION_ERROR",
            static_cast<int>(web::GeodeWebError::CURL_INITIALIZATION_ERROR)
        );
        setIntField(L, "REQUEST_CANCELLED", static_cast<int>(web::GeodeWebError::REQUEST_CANCELLED));
        setIntField(L, "QUEUE_FULL", static_cast<int>(web::GeodeWebError::QUEUE_FULL));
        setIntField(L, "CHANNEL_CLOSED", static_cast<int>(web::GeodeWebError::CHANNEL_CLOSED));
        lua_setfield(L, -2, "Error");

        lua_createtable(L, 0, 11);
        setLongField(L, "BASIC", web::http_auth::BASIC);
        setLongField(L, "DIGEST", web::http_auth::DIGEST);
        setLongField(L, "DIGEST_IE", web::http_auth::DIGEST_IE);
        setLongField(L, "BEARER", web::http_auth::BEARER);
        setLongField(L, "NEGOTIATE", web::http_auth::NEGOTIATE);
        setLongField(L, "NTLM", web::http_auth::NTLM);
        setLongField(L, "NTLM_WB", web::http_auth::NTLM_WB);
        setLongField(L, "ANY", web::http_auth::ANY);
        setLongField(L, "ANYSAFE", web::http_auth::ANYSAFE);
        setLongField(L, "ONLY", web::http_auth::ONLY);
        setLongField(L, "AWS_SIGV4", web::http_auth::AWS_SIGV4);
        lua_setfield(L, -2, "HttpAuth");
    }
} // namespace luax::webdetail
