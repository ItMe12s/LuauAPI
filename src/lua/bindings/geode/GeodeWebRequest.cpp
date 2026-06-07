#include "lua/bindings/framework/LuaCallback.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/CurrentMod.hpp"
#include "lua/bindings/geode/GeodeWebInternal.hpp"
#include "lua/bindings/geode/JsonConvert.hpp"
#include "lua/bindings/geode/WebCaps.hpp"

#include <Geode/utils/web.hpp>
#include <chrono>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace luax::webdetail {
    using namespace luax;

    namespace {
        int requestChain(lua_State* L, auto&& fn, char const* method) {
            auto& req = checkRequest(L, 1, method);
            fn(req);
            lua_pushvalue(L, 1);
            return 1;
        }

        int requestSendWithMethod(lua_State* L, std::string method, char const* context) {
            auto& req = checkRequest(L, 1, context);
            auto url = check<std::string>(L, 2, context);
            int optionsIdx = 0;
            int callbackIdx = 0;
            parseRequestCall(L, 3, 4, optionsIdx, callbackIdx);
            applyOptions(L, req, optionsIdx, context);
            auto task = startRequest(L, req, std::move(method), std::move(url), callbackIdx);
            pushHandle(L, std::move(task));
            return 1;
        }

        int sendConvenience(lua_State* L, std::string method, char const* context) {
            auto url = check<std::string>(L, 1, context);
            int optionsIdx = 0;
            int callbackIdx = 0;
            parseRequestCall(L, 2, 3, optionsIdx, callbackIdx);
            web::WebRequest req;
            applyOptions(L, req, optionsIdx, context);
            auto task = startRequest(L, req, std::move(method), std::move(url), callbackIdx);
            pushHandle(L, std::move(task));
            return 1;
        }
    } // namespace

    int parseRequestCall(lua_State* L, int optionsIdx, int callbackIdx, int& outOptions, int& outCallback) {
        if (lua_isfunction(L, optionsIdx)) {
            outOptions = 0;
            outCallback = optionsIdx;
            return outCallback;
        }
        outOptions = optionsIdx;
        outCallback = callbackIdx;
        luaL_checktype(L, outCallback, LUA_TFUNCTION);
        return outCallback;
    }

    std::shared_ptr<WebTask> startRequest(
        lua_State* L, web::WebRequest& req, std::string method, std::string url, int callbackIdx
    ) {
        luaL_checktype(L, callbackIdx, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, callbackIdx);
        auto task = std::make_shared<WebTask>(req.getID());
        activeTasks().push_back(task);
        compactWeakState();
        ensureShutdownHook();

        auto* mod = requireCurrentMod(L);
        auto future = req.send(std::move(method), std::move(url), mod);
        task->holder.spawn(
            "LuauAPI web request", std::move(future), [cb, task](web::WebResponse response) mutable {
                task->done = true;
                if (!cb || !cb->valid()) return;
                struct Ctx {
                    web::WebResponse* response;
                } ctx{&response};
                if (!cb->invoke(
                        2,
                        0,
                        "geode.utils.web request",
                        kHookScriptDeadlineMs,
                        +[](lua_State* L, void* raw) {
                            auto& response = *static_cast<Ctx*>(raw)->response;
                            if (!responseDataWithinLimit(response.data().size())) {
                                lua_pushnil(L);
                                push(L, std::string(kWebResponseSizeExceededMsg));
                                return;
                            }
                            pushResponseOrError(L, std::move(response));
                        },
                        &ctx
                    )) {
                    logWebCallbackFailure("geode.utils.web request");
                }
            }
        );
        return task;
    }

    int webOpenLinkInBrowser(lua_State* L) {
        auto url = check<std::string>(L, 1, "geode.utils.web.openLinkInBrowser");
        web::openLinkInBrowser(url);
        return 0;
    }

    int webNewRequest(lua_State* L) {
        pushRequest(L, web::WebRequest());
        return 1;
    }

    int webMultipart(lua_State* L) {
        pushMultipart(L);
        return 1;
    }

    int webGet(lua_State* L) {
        return sendConvenience(L, "GET", "geode.utils.web.get");
    }

    int webPost(lua_State* L) {
        return sendConvenience(L, "POST", "geode.utils.web.post");
    }

    int webPut(lua_State* L) {
        return sendConvenience(L, "PUT", "geode.utils.web.put");
    }

    int webPatch(lua_State* L) {
        return sendConvenience(L, "PATCH", "geode.utils.web.patch");
    }

    int webFetch(lua_State* L) {
        web::WebRequest req;
        std::string url;
        std::string method = "GET";
        int optionsIdx = 0;
        int callbackIdx = 0;

        if (lua_istable(L, 1)) {
            optionsIdx = 1;
            callbackIdx = 2;
            luaL_checktype(L, callbackIdx, LUA_TFUNCTION);
            auto maybeUrl = optStringField(L, 1, "url", "geode.utils.web.fetch");
            if (!maybeUrl) luaL_error(L, "geode.utils.web.fetch expected url field");
            url = std::move(*maybeUrl);
            if (auto value = optStringField(L, 1, "method", "geode.utils.web.fetch"))
                method = std::move(*value);
        }
        else {
            url = check<std::string>(L, 1, "geode.utils.web.fetch");
            parseRequestCall(L, 2, 3, optionsIdx, callbackIdx);
            if (optionsIdx > 0) {
                if (auto value = optStringField(L, optionsIdx, "method", "geode.utils.web.fetch")) {
                    method = std::move(*value);
                }
            }
        }

        applyOptions(L, req, optionsIdx, "geode.utils.web.fetch");
        auto task = startRequest(L, req, std::move(method), std::move(url), callbackIdx);
        pushHandle(L, std::move(task));
        return 1;
    }

    int requestHeader(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebRequest:header");
        auto value = check<std::string>(L, 3, "WebRequest:header");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.header(std::move(name), std::move(value));
            },
            "WebRequest:header"
        );
    }

    int requestRemoveHeader(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebRequest:removeHeader");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.removeHeader(name);
            },
            "WebRequest:removeHeader"
        );
    }

    int requestParam(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebRequest:param");
        auto value = check<std::string>(L, 3, "WebRequest:param");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.param(std::move(name), std::move(value));
            },
            "WebRequest:param"
        );
    }

    int requestRemoveParam(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebRequest:removeParam");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.removeParam(name);
            },
            "WebRequest:removeParam"
        );
    }

    int requestMethod(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:method");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.method(std::move(value));
            },
            "WebRequest:method"
        );
    }

    int requestUrl(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:url");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.url(std::move(value));
            },
            "WebRequest:url"
        );
    }

    int requestUserAgent(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:userAgent");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.userAgent(std::move(value));
            },
            "WebRequest:userAgent"
        );
    }

    int requestAcceptEncoding(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:acceptEncoding");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.acceptEncoding(std::move(value));
            },
            "WebRequest:acceptEncoding"
        );
    }

    int requestTimeout(lua_State* L) {
        auto seconds = check<int>(L, 2, "WebRequest:timeout");
        if (seconds < 0) luaL_error(L, "WebRequest:timeout expected seconds >= 0");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.timeout(std::chrono::seconds(seconds));
            },
            "WebRequest:timeout"
        );
    }

    int requestDownloadRange(lua_State* L) {
        if (!lua_isnumber(L, 2) || lua_tointeger(L, 2) < 0)
            luaL_error(L, "WebRequest:downloadRange expected non-negative integer at arg 2");
        if (!lua_isnumber(L, 3) || lua_tointeger(L, 3) < 0)
            luaL_error(L, "WebRequest:downloadRange expected non-negative integer at arg 3");
        auto start = static_cast<std::uint64_t>(lua_tointeger(L, 2));
        auto stop = static_cast<std::uint64_t>(lua_tointeger(L, 3));
        if (start > stop) luaL_error(L, "WebRequest:downloadRange expected start <= stop");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.downloadRange({start, stop});
            },
            "WebRequest:downloadRange"
        );
    }

    int requestCertVerification(lua_State* L) {
        auto value = check<bool>(L, 2, "WebRequest:certVerification");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.certVerification(value);
            },
            "WebRequest:certVerification"
        );
    }

    int requestTransferBody(lua_State* L) {
        auto value = check<bool>(L, 2, "WebRequest:transferBody");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.transferBody(value);
            },
            "WebRequest:transferBody"
        );
    }

    int requestFollowRedirects(lua_State* L) {
        auto value = check<bool>(L, 2, "WebRequest:followRedirects");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.followRedirects(value);
            },
            "WebRequest:followRedirects"
        );
    }

    int requestIgnoreContentLength(lua_State* L) {
        auto value = check<bool>(L, 2, "WebRequest:ignoreContentLength");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.ignoreContentLength(value);
            },
            "WebRequest:ignoreContentLength"
        );
    }

    int requestCaBundle(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:caBundle");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.CABundleContent(std::move(value));
            },
            "WebRequest:caBundle"
        );
    }

    int requestProxy(lua_State* L) {
        auto opts = checkProxyOpts(L, 2, "WebRequest:proxy");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.proxyOpts(std::move(opts));
            },
            "WebRequest:proxy"
        );
    }

    int requestVersion(lua_State* L) {
        auto version = checkHttpVersion(L, 2, "WebRequest:version");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.version(version);
            },
            "WebRequest:version"
        );
    }

    int requestBody(lua_State* L) {
        auto data = check<std::string>(L, 2, "WebRequest:body");
        if (!requestBodyWithinLimit(data.size())) {
            return pushRequestBodyExceeded(L);
        }
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                geode::ByteVector bytes(data.begin(), data.end());
                req.body(std::move(bytes));
            },
            "WebRequest:body"
        );
    }

    int requestBodyString(lua_State* L) {
        auto data = check<std::string>(L, 2, "WebRequest:bodyString");
        if (!requestBodyWithinLimit(data.size())) {
            return pushRequestBodyExceeded(L);
        }
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.bodyString(data);
            },
            "WebRequest:bodyString"
        );
    }

    int requestBodyJson(lua_State* L) {
        auto value = toJson(L, 2, 0);
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.bodyJSON(value);
            },
            "WebRequest:bodyJson"
        );
    }

    int requestBodyMultipart(lua_State* L) {
        auto& form = checkMultipartBox(L, 2, "WebRequest:bodyMultipart")->form;
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.bodyMultipart(form);
            },
            "WebRequest:bodyMultipart"
        );
    }

    int requestOnProgress(lua_State* L) {
        auto* box = checkRequestBox(L, 1, "WebRequest:onProgress");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        box->progress = std::make_shared<LuaCallback>(L, 2);
        setProgressCallback(box->request, box->progress);
        lua_pushvalue(L, 1);
        return 1;
    }

    int requestSend(lua_State* L) {
        auto& req = checkRequest(L, 1, "WebRequest:send");
        auto method = check<std::string>(L, 2, "WebRequest:send");
        auto url = check<std::string>(L, 3, "WebRequest:send");
        int optionsIdx = 0;
        int callbackIdx = 0;
        parseRequestCall(L, 4, 5, optionsIdx, callbackIdx);
        applyOptions(L, req, optionsIdx, "WebRequest:send");
        auto task = startRequest(L, req, std::move(method), std::move(url), callbackIdx);
        pushHandle(L, std::move(task));
        return 1;
    }

    int requestGet(lua_State* L) {
        return requestSendWithMethod(L, "GET", "WebRequest:get");
    }

    int requestPost(lua_State* L) {
        return requestSendWithMethod(L, "POST", "WebRequest:post");
    }

    int requestPut(lua_State* L) {
        return requestSendWithMethod(L, "PUT", "WebRequest:put");
    }

    int requestPatch(lua_State* L) {
        return requestSendWithMethod(L, "PATCH", "WebRequest:patch");
    }

    int requestGetID(lua_State* L) {
        push(L, static_cast<double>(checkRequest(L, 1, "WebRequest:getID").getID()));
        return 1;
    }

    int requestGetMethod(lua_State* L) {
        push(L, std::string_view(checkRequest(L, 1, "WebRequest:getMethod").getMethod()));
        return 1;
    }

    int requestGetUrl(lua_State* L) {
        push(L, std::string_view(checkRequest(L, 1, "WebRequest:getUrl").getUrl()));
        return 1;
    }

    int requestGetHeaders(lua_State* L) {
        auto const& headers = checkRequest(L, 1, "WebRequest:getHeaders").getHeaders();
        lua_createtable(L, 0, static_cast<int>(headers.size()));
        for (auto const& [name, values] : headers) {
            lua_createtable(L, static_cast<int>(values.size()), 0);
            int i = 1;
            for (auto const& value : values) {
                push(L, value);
                lua_rawseti(L, -2, i++);
            }
            lua_setfield(L, -2, name.c_str());
        }
        return 1;
    }

    int requestGetUrlParams(lua_State* L) {
        auto const& params = checkRequest(L, 1, "WebRequest:getUrlParams").getUrlParams();
        lua_createtable(L, 0, static_cast<int>(params.size()));
        for (auto const& [name, value] : params) {
            push(L, value);
            lua_setfield(L, -2, name.c_str());
        }
        return 1;
    }

    int requestGetBody(lua_State* L) {
        auto body = checkRequest(L, 1, "WebRequest:getBody").getBody();
        if (!body) {
            lua_pushnil(L);
            return 1;
        }
        lua_pushlstring(L, reinterpret_cast<char const*>(body->data()), body->size());
        return 1;
    }

    int requestGetTimeout(lua_State* L) {
        auto timeout = checkRequest(L, 1, "WebRequest:getTimeout").getTimeout();
        if (!timeout) {
            lua_pushnil(L);
            return 1;
        }
        push(L, static_cast<double>(timeout->count()));
        return 1;
    }

    int requestGetVersion(lua_State* L) {
        push(L, static_cast<int>(checkRequest(L, 1, "WebRequest:getVersion").getHttpVersion()));
        return 1;
    }

    int requestGetProgress(lua_State* L) {
        pushProgress(L, checkRequest(L, 1, "WebRequest:getProgress").getProgress());
        return 1;
    }
} // namespace luax::webdetail
