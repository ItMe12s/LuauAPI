#pragma once

#include "bindings/geode/web/WebInternal.hpp"
#include "framework/callback/LuaCallback.hpp"

#include <Geode/utils/web.hpp>
#include <cstdint>
#include <functional>
#include <lua.h>
#include <matjson.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace luax::webdetail {
    void logWebCallbackFailure(char const* context);

    int optPriority(lua_State* L, int idx);
    void applyOptions(lua_State* L, web::WebRequest& req, int idx, char const* method);

    void applyScalarOptions(lua_State* L, web::WebRequest& req, int idx, char const* method);
    bool applyBody(web::WebRequest& req, std::string const& data);
    bool applyBodyString(web::WebRequest& req, std::string const& data);
    bool applyBodyJson(web::WebRequest& req, matjson::Value value);
    bool applyBodyMultipart(web::WebRequest& req, web::MultipartForm const& form);
    web::ProxyOpts checkProxyOpts(lua_State* L, int idx, char const* method);
    web::HttpVersion checkHttpVersion(lua_State* L, int idx, char const* method);
    void setProgressCallback(web::WebRequest& req, std::shared_ptr<LuaCallback> cb);
    void pushProgress(lua_State* L, web::WebProgress const& progress);
    std::uint64_t checkNonNegativeInteger(lua_State* L, int idx, char const* method);

    std::shared_ptr<WebTask> startRequest(
        lua_State* L, web::WebRequest& req, std::string method, std::string url, int callbackIdx
    );

    int parseRequestCall(lua_State* L, int optionsIdx, int callbackIdx, int& outOptions, int& outCallback);

    int webOpenLinkInBrowser(lua_State* L);
    int webNewRequest(lua_State* L);
    int webMultipart(lua_State* L);
    int webGet(lua_State* L);
    int webPost(lua_State* L);
    int webPut(lua_State* L);
    int webPatch(lua_State* L);
    int webFetch(lua_State* L);

    int requestHeader(lua_State* L);
    int requestRemoveHeader(lua_State* L);
    int requestParam(lua_State* L);
    int requestRemoveParam(lua_State* L);
    int requestMethod(lua_State* L);
    int requestUrl(lua_State* L);
    int requestUserAgent(lua_State* L);
    int requestAcceptEncoding(lua_State* L);
    int requestTimeout(lua_State* L);
    int requestDownloadRange(lua_State* L);
    int requestCertVerification(lua_State* L);
    int requestTransferBody(lua_State* L);
    int requestFollowRedirects(lua_State* L);
    int requestIgnoreContentLength(lua_State* L);
    int requestCaBundle(lua_State* L);
    int requestProxy(lua_State* L);
    int requestVersion(lua_State* L);
    int requestBody(lua_State* L);
    int requestBodyString(lua_State* L);
    int requestBodyJson(lua_State* L);
    int requestBodyMultipart(lua_State* L);
    int requestOnProgress(lua_State* L);
    int requestSend(lua_State* L);
    int requestGet(lua_State* L);
    int requestPost(lua_State* L);
    int requestPut(lua_State* L);
    int requestPatch(lua_State* L);
    int requestGetID(lua_State* L);
    int requestGetMethod(lua_State* L);
    int requestGetUrl(lua_State* L);
    int requestGetHeaders(lua_State* L);
    int requestGetUrlParams(lua_State* L);
    int requestGetBody(lua_State* L);
    int requestGetTimeout(lua_State* L);
    int requestGetVersion(lua_State* L);
    int requestGetProgress(lua_State* L);

    int responseInfo(lua_State* L);
    int responseOk(lua_State* L);
    int responseRedirected(lua_State* L);
    int responseBadClient(lua_State* L);
    int responseBadServer(lua_State* L);
    int responseError(lua_State* L);
    int responseCancelled(lua_State* L);
    int responseCode(lua_State* L);
    int responseText(lua_State* L);
    int responseJson(lua_State* L);
    int responseBytes(lua_State* L);
    int responseSaveTo(lua_State* L);
    int responseHeaders(lua_State* L);
    int responseHeader(lua_State* L);
    int responseHeadersNamed(lua_State* L);
    int responseErrorMessage(lua_State* L);
    int responseVerboseLogs(lua_State* L);
    int responseTimings(lua_State* L);

    int multipartParam(lua_State* L);
    int multipartFile(lua_State* L);
    int multipartFileFrom(lua_State* L);
    int multipartGetBoundary(lua_State* L);
    int multipartGetHeader(lua_State* L);
    int multipartGetBody(lua_State* L);

    int handleCancel(lua_State* L);
    int handleId(lua_State* L);
    int listenerDisconnect(lua_State* L);

    int webOnRequestIntercept(lua_State* L);
    int webOnRequestInterceptFor(lua_State* L);
    int webOnRequestInterceptById(lua_State* L);
    int webOnResponse(lua_State* L);
    int webOnResponseFor(lua_State* L);
    int webOnResponseById(lua_State* L);

    void registerConstants(lua_State* L);
    void registerMetatables(lua_State* L);
} // namespace luax::webdetail
