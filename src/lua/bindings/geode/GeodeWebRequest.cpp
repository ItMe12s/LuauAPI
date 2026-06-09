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
        enum class ChainOp : std::uint8_t {
            Header,
            RemoveHeader,
            Param,
            RemoveParam,
            Method,
            Url,
            UserAgent,
            AcceptEncoding,
            Timeout,
            DownloadRange,
            CertVerification,
            TransferBody,
            FollowRedirects,
            IgnoreContentLength,
            CaBundle,
            Proxy,
            Version,
        };

        struct RequestChainDescriptor {
            char const* context;
            ChainOp op;
        };

        void applyRequestChain(lua_State* L, web::WebRequest& req, RequestChainDescriptor const& desc) {
            switch (desc.op) {
                case ChainOp::Header: {
                    auto name = check<std::string>(L, 2, desc.context);
                    auto value = check<std::string>(L, 3, desc.context);
                    req.header(std::move(name), std::move(value));
                    return;
                }
                case ChainOp::RemoveHeader:
                    req.removeHeader(check<std::string>(L, 2, desc.context));
                    return;
                case ChainOp::Param: {
                    auto name = check<std::string>(L, 2, desc.context);
                    auto value = check<std::string>(L, 3, desc.context);
                    req.param(std::move(name), std::move(value));
                    return;
                }
                case ChainOp::RemoveParam:
                    req.removeParam(check<std::string>(L, 2, desc.context));
                    return;
                case ChainOp::Method:
                    req.method(check<std::string>(L, 2, desc.context));
                    return;
                case ChainOp::Url:
                    req.url(check<std::string>(L, 2, desc.context));
                    return;
                case ChainOp::UserAgent:
                    req.userAgent(check<std::string>(L, 2, desc.context));
                    return;
                case ChainOp::AcceptEncoding:
                    req.acceptEncoding(check<std::string>(L, 2, desc.context));
                    return;
                case ChainOp::Timeout: {
                    auto seconds = check<int>(L, 2, desc.context);
                    if (seconds < 0) luaL_error(L, "WebRequest:timeout expected seconds >= 0");
                    req.timeout(std::chrono::seconds(static_cast<std::int64_t>(seconds)));
                    return;
                }
                case ChainOp::DownloadRange:
                    if (!lua_isnumber(L, 2) || lua_tointeger(L, 2) < 0)
                        luaL_error(L, "WebRequest:downloadRange expected non-negative integer at arg 2");
                    if (!lua_isnumber(L, 3) || lua_tointeger(L, 3) < 0)
                        luaL_error(L, "WebRequest:downloadRange expected non-negative integer at arg 3");
                    {
                        auto start = static_cast<std::uint64_t>(lua_tointeger(L, 2));
                        auto stop = static_cast<std::uint64_t>(lua_tointeger(L, 3));
                        if (start > stop)
                            luaL_error(L, "WebRequest:downloadRange expected start <= stop");
                        req.downloadRange({start, stop});
                    }
                    return;
                case ChainOp::CertVerification:
                    req.certVerification(check<bool>(L, 2, desc.context));
                    return;
                case ChainOp::TransferBody:
                    req.transferBody(check<bool>(L, 2, desc.context));
                    return;
                case ChainOp::FollowRedirects:
                    req.followRedirects(check<bool>(L, 2, desc.context));
                    return;
                case ChainOp::IgnoreContentLength:
                    req.ignoreContentLength(check<bool>(L, 2, desc.context));
                    return;
                case ChainOp::CaBundle:
                    req.CABundleContent(check<std::string>(L, 2, desc.context));
                    return;
                case ChainOp::Proxy:
                    req.proxyOpts(checkProxyOpts(L, 2, desc.context));
                    return;
                case ChainOp::Version:
                    req.version(checkHttpVersion(L, 2, desc.context));
                    return;
            }
        }

        int requestMethodFromTable(lua_State* L, RequestChainDescriptor const& desc) {
            auto& req = checkRequest(L, 1, desc.context);
            applyRequestChain(L, req, desc);
            lua_pushvalue(L, 1);
            return 1;
        }

        RequestChainDescriptor const kRequestChainDescriptors[] = {
            {"WebRequest:header", ChainOp::Header},
            {"WebRequest:removeHeader", ChainOp::RemoveHeader},
            {"WebRequest:param", ChainOp::Param},
            {"WebRequest:removeParam", ChainOp::RemoveParam},
            {"WebRequest:method", ChainOp::Method},
            {"WebRequest:url", ChainOp::Url},
            {"WebRequest:userAgent", ChainOp::UserAgent},
            {"WebRequest:acceptEncoding", ChainOp::AcceptEncoding},
            {"WebRequest:timeout", ChainOp::Timeout},
            {"WebRequest:downloadRange", ChainOp::DownloadRange},
            {"WebRequest:certVerification", ChainOp::CertVerification},
            {"WebRequest:transferBody", ChainOp::TransferBody},
            {"WebRequest:followRedirects", ChainOp::FollowRedirects},
            {"WebRequest:ignoreContentLength", ChainOp::IgnoreContentLength},
            {"WebRequest:caBundle", ChainOp::CaBundle},
            {"WebRequest:proxy", ChainOp::Proxy},
            {"WebRequest:version", ChainOp::Version},
        };

        enum class RequestChainId : std::size_t {
            Header = 0,
            RemoveHeader,
            Param,
            RemoveParam,
            Method,
            Url,
            UserAgent,
            AcceptEncoding,
            Timeout,
            DownloadRange,
            CertVerification,
            TransferBody,
            FollowRedirects,
            IgnoreContentLength,
            CaBundle,
            Proxy,
            Version,
        };

        int requestChainDispatch(lua_State* L, RequestChainId id) {
            return requestMethodFromTable(L, kRequestChainDescriptors[static_cast<std::size_t>(id)]);
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
                                pushNilErrCallback(L, kWebResponseSizeExceededMsg);
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
        return requestChainDispatch(L, RequestChainId::Header);
    }

    int requestRemoveHeader(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::RemoveHeader);
    }

    int requestParam(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::Param);
    }

    int requestRemoveParam(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::RemoveParam);
    }

    int requestMethod(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::Method);
    }

    int requestUrl(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::Url);
    }

    int requestUserAgent(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::UserAgent);
    }

    int requestAcceptEncoding(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::AcceptEncoding);
    }

    int requestTimeout(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::Timeout);
    }

    int requestDownloadRange(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::DownloadRange);
    }

    int requestCertVerification(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::CertVerification);
    }

    int requestTransferBody(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::TransferBody);
    }

    int requestFollowRedirects(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::FollowRedirects);
    }

    int requestIgnoreContentLength(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::IgnoreContentLength);
    }

    int requestCaBundle(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::CaBundle);
    }

    int requestProxy(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::Proxy);
    }

    int requestVersion(lua_State* L) {
        return requestChainDispatch(L, RequestChainId::Version);
    }

    int requestBody(lua_State* L) {
        auto data = check<std::string>(L, 2, "WebRequest:body");
        auto& req = checkRequest(L, 1, "WebRequest:body");
        if (!applyBody(req, data)) return pushRequestBodyExceeded(L);
        lua_pushvalue(L, 1);
        return 1;
    }

    int requestBodyString(lua_State* L) {
        auto data = check<std::string>(L, 2, "WebRequest:bodyString");
        auto& req = checkRequest(L, 1, "WebRequest:bodyString");
        if (!applyBodyString(req, data)) return pushRequestBodyExceeded(L);
        lua_pushvalue(L, 1);
        return 1;
    }

    int requestBodyJson(lua_State* L) {
        auto valueResult = toJson(L, 2, 0);
        if (valueResult.isErr()) {
            return pushNilErr(L, valueResult.unwrapErr());
        }
        auto& req = checkRequest(L, 1, "WebRequest:bodyJson");
        if (!applyBodyJson(req, std::move(valueResult.unwrap()))) return pushRequestBodyExceeded(L);
        lua_pushvalue(L, 1);
        return 1;
    }

    int requestBodyMultipart(lua_State* L) {
        auto& form = checkMultipartBox(L, 2, "WebRequest:bodyMultipart")->form;
        auto& req = checkRequest(L, 1, "WebRequest:bodyMultipart");
        if (!applyBodyMultipart(req, form)) return pushRequestBodyExceeded(L);
        lua_pushvalue(L, 1);
        return 1;
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
