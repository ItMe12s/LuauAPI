#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/GeodeWebInternal.hpp"
#include "lua/bindings/geode/WebInternal.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>

namespace luax::webdetail {
    using namespace luax;

    namespace {
        void registerMethods(lua_State* L, char const* meta, luaL_Reg const* methods, lua_CFunction gc) {
            if (luaL_newmetatable(L, meta)) {
                for (luaL_Reg const* reg = methods; reg->name != nullptr; ++reg) {
                    setTableCFunction(L, -1, reg->name, reg->func);
                }
                lua_pushcfunction(L, gc, "__gc");
                lua_setfield(L, -2, "__gc");
                lua_pushvalue(L, -1);
                lua_setfield(L, -2, "__index");
                lua_pushstring(L, "locked");
                lua_setfield(L, -2, "__metatable");
            }
            lua_pop(L, 1);
        }
    } // namespace

    void registerMetatables(lua_State* L) {
        luaL_Reg requestMethods[] = {
            {"header", requestHeader},
            {"removeHeader", requestRemoveHeader},
            {"param", requestParam},
            {"removeParam", requestRemoveParam},
            {"method", requestMethod},
            {"url", requestUrl},
            {"userAgent", requestUserAgent},
            {"acceptEncoding", requestAcceptEncoding},
            {"timeout", requestTimeout},
            {"downloadRange", requestDownloadRange},
            {"certVerification", requestCertVerification},
            {"transferBody", requestTransferBody},
            {"followRedirects", requestFollowRedirects},
            {"ignoreContentLength", requestIgnoreContentLength},
            {"caBundle", requestCaBundle},
            {"proxy", requestProxy},
            {"version", requestVersion},
            {"body", requestBody},
            {"bodyString", requestBodyString},
            {"bodyJson", requestBodyJson},
            {"bodyMultipart", requestBodyMultipart},
            {"onProgress", requestOnProgress},
            {"send", requestSend},
            {"get", requestGet},
            {"post", requestPost},
            {"put", requestPut},
            {"patch", requestPatch},
            {"getID", requestGetID},
            {"getMethod", requestGetMethod},
            {"getUrl", requestGetUrl},
            {"getHeaders", requestGetHeaders},
            {"getUrlParams", requestGetUrlParams},
            {"getBody", requestGetBody},
            {"getTimeout", requestGetTimeout},
            {"getVersion", requestGetVersion},
            {"getProgress", requestGetProgress},
            {nullptr, nullptr},
        };
        registerMethods(L, kRequestMeta, requestMethods, &requestGc);

        luaL_Reg responseMethods[] = {
            {"info", responseInfo},
            {"ok", responseOk},
            {"redirected", responseRedirected},
            {"badClient", responseBadClient},
            {"badServer", responseBadServer},
            {"error", responseError},
            {"cancelled", responseCancelled},
            {"code", responseCode},
            {"text", responseText},
            {"json", responseJson},
            {"bytes", responseBytes},
            {"saveTo", responseSaveTo},
            {"headers", responseHeaders},
            {"header", responseHeader},
            {"headersNamed", responseHeadersNamed},
            {"errorMessage", responseErrorMessage},
            {"verboseLogs", responseVerboseLogs},
            {"timings", responseTimings},
            {nullptr, nullptr},
        };
        registerMethods(L, kResponseMeta, responseMethods, &responseGc);

        luaL_Reg multipartMethods[] = {
            {"param", multipartParam},
            {"file", multipartFile},
            {"fileFrom", multipartFileFrom},
            {"getBoundary", multipartGetBoundary},
            {"getHeader", multipartGetHeader},
            {"getBody", multipartGetBody},
            {nullptr, nullptr},
        };
        registerMethods(L, kMultipartMeta, multipartMethods, &multipartGc);

        luaL_Reg handleMethods[] = {
            {"cancel", handleCancel},
            {"id", handleId},
            {nullptr, nullptr},
        };
        registerMethods(L, kHandleMeta, handleMethods, &handleGc);

        luaL_Reg listenerMethods[] = {
            {"disconnect", listenerDisconnect},
            {nullptr, nullptr},
        };
        registerMethods(L, kListenerMeta, listenerMethods, &listenerGc);
    }
} // namespace luax::webdetail

namespace luax {
    geode::Result<void> registerGeodeWeb(lua_State* L) {
        webdetail::registerMetatables(L);

        getOrCreateTable(L, "geode.utils.web");
        setTableCFunction(L, -1, "openLinkInBrowser", &webdetail::webOpenLinkInBrowser);
        setTableCFunction(L, -1, "newRequest", &webdetail::webNewRequest);
        setTableCFunction(L, -1, "multipart", &webdetail::webMultipart);
        setTableCFunction(L, -1, "get", &webdetail::webGet);
        setTableCFunction(L, -1, "post", &webdetail::webPost);
        setTableCFunction(L, -1, "put", &webdetail::webPut);
        setTableCFunction(L, -1, "patch", &webdetail::webPatch);
        setTableCFunction(L, -1, "fetch", &webdetail::webFetch);
        setTableCFunction(L, -1, "onRequestIntercept", &webdetail::webOnRequestIntercept);
        setTableCFunction(L, -1, "onRequestInterceptFor", &webdetail::webOnRequestInterceptFor);
        setTableCFunction(L, -1, "onRequestInterceptById", &webdetail::webOnRequestInterceptById);
        setTableCFunction(L, -1, "onResponse", &webdetail::webOnResponse);
        setTableCFunction(L, -1, "onResponseFor", &webdetail::webOnResponseFor);
        setTableCFunction(L, -1, "onResponseById", &webdetail::webOnResponseById);
        webdetail::registerConstants(L);
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_web_lib, luax::registerGeodeWeb)
#endif
