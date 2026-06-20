#include "bindings/geode/JsonConvert.hpp"
#include "bindings/geode/ModSandbox.hpp"
#include "bindings/geode/web/GeodeWebInternal.hpp"
#include "bindings/geode/web/WebCaps.hpp"
#include "bindings/geode/web/WebInternal.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Priority.hpp>
#include <Geode/utils/web.hpp>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <matjson.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <utility>

namespace luax::webdetail {
    using namespace luax;

    namespace {
        using StringMoveFn = web::WebRequest& (web::WebRequest::*)(std::string);
        using BoolFn = web::WebRequest& (web::WebRequest::*)(bool);

        struct StringOptionDescriptor {
            char const* field;
            StringMoveFn setter;
        };

        struct BoolOptionDescriptor {
            char const* field;
            BoolFn setter;
        };

        StringOptionDescriptor const kStringOptions[] = {
            {"method", &web::WebRequest::method},
            {"url", &web::WebRequest::url},
            {"userAgent", &web::WebRequest::userAgent},
            {"acceptEncoding", &web::WebRequest::acceptEncoding},
            {"caBundle", &web::WebRequest::CABundleContent},
        };

        BoolOptionDescriptor const kBoolOptions[] = {
            {"certVerification", &web::WebRequest::certVerification},
            {"transferBody", &web::WebRequest::transferBody},
            {"followRedirects", &web::WebRequest::followRedirects},
            {"ignoreContentLength", &web::WebRequest::ignoreContentLength},
        };

        void applyStringMap(lua_State* L, int idx, char const* field, char const* method, auto&& setter) {
            lua_getfield(L, idx, field);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return;
            }
            luaL_checktype(L, -1, LUA_TTABLE);
            int table = lua_absindex(L, -1);
            lua_pushnil(L);
            while (lua_next(L, table) != 0) {
                auto key = check<std::string>(L, -2, method);
                auto value = check<std::string>(L, -1, method);
                setter(std::move(key), std::move(value));
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }

        std::optional<std::pair<std::uint64_t, std::uint64_t>> optDownloadRange(
            lua_State* L, int idx, char const* method
        ) {
            lua_getfield(L, idx, "downloadRange");
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return std::nullopt;
            }
            luaL_checktype(L, -1, LUA_TTABLE);
            lua_rawgeti(L, -1, 1);
            auto start = checkNonNegativeInteger(L, -1, method);
            lua_pop(L, 1);
            lua_rawgeti(L, -1, 2);
            auto stop = checkNonNegativeInteger(L, -1, method);
            lua_pop(L, 2);
            if (start > stop) luaL_error(L, "%s expected downloadRange start <= stop", method);
            return std::pair<std::uint64_t, std::uint64_t>{start, stop};
        }
    } // namespace

    void applyScalarOptions(lua_State* L, web::WebRequest& req, int idx, char const* method) {
        applyStringMap(L, idx, "headers", method, [&](std::string name, std::string value) {
            req.header(std::move(name), std::move(value));
        });
        applyStringMap(L, idx, "params", method, [&](std::string name, std::string value) {
            req.param(std::move(name), std::move(value));
        });

        for (auto const& desc : kStringOptions) {
            if (auto value = optStringField(L, idx, desc.field, method))
                (req.*desc.setter)(std::move(*value));
        }

        if (auto value = optNumberField(L, idx, "timeout", method)) {
            if (*value < 0) luaL_error(L, "%s expected timeout >= 0", method);
            req.timeout(std::chrono::seconds(static_cast<std::int64_t>(*value)));
        }
        if (auto value = optDownloadRange(L, idx, method))
            req.downloadRange({value->first, value->second});

        for (auto const& desc : kBoolOptions) {
            if (auto value = optBoolField(L, idx, desc.field, method)) (req.*desc.setter)(*value);
        }

        lua_getfield(L, idx, "proxy");
        if (!lua_isnil(L, -1)) req.proxyOpts(checkProxyOpts(L, -1, method));
        lua_pop(L, 1);

        lua_getfield(L, idx, "version");
        if (!lua_isnil(L, -1)) req.version(checkHttpVersion(L, -1, method));
        lua_pop(L, 1);
    }

    bool applyBody(web::WebRequest& req, std::string const& data) {
        if (!requestBodyWithinLimit(data.size())) return false;
        geode::ByteVector bytes(data.begin(), data.end());
        req.body(std::move(bytes));
        return true;
    }

    bool applyBodyString(web::WebRequest& req, std::string const& data) {
        if (!requestBodyWithinLimit(data.size())) return false;
        req.bodyString(data);
        return true;
    }

    bool applyBodyJson(web::WebRequest& req, matjson::Value value) {
        if (!requestJsonBodyWithinLimit(value)) return false;
        req.bodyJSON(std::move(value));
        return true;
    }

    bool applyBodyMultipart(web::WebRequest& req, web::MultipartForm const& form) {
        if (!requestBodyWithinLimit(form.getBody().size())) return false;
        req.bodyMultipart(form);
        return true;
    }

    namespace {
        web::ProxyType checkProxyType(lua_State* L, int idx, char const* method) {
            int value = check<int>(L, idx, method);
            if (value < static_cast<int>(web::ProxyType::HTTP) ||
                value > static_cast<int>(web::ProxyType::SOCKS5H)) {
                luaL_error(L, "%s: invalid ProxyType %d", method, value);
            }
            return static_cast<web::ProxyType>(value);
        }

    } // namespace

    int optPriority(lua_State* L, int idx) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return geode::Priority::Normal;
        return check<int>(L, idx, "geode.utils.web listener");
    }

    std::uint64_t checkNonNegativeInteger(lua_State* L, int idx, char const* method) {
        if (!lua_isnumber(L, idx))
            luaL_error(L, "%s expected non-negative integer at arg %d", method, idx);
        auto value = lua_tointeger(L, idx);
        if (value < 0) luaL_error(L, "%s expected non-negative integer at arg %d", method, idx);
        return static_cast<std::uint64_t>(value);
    }

    web::HttpVersion checkHttpVersion(lua_State* L, int idx, char const* method) {
        int value = check<int>(L, idx, method);
        switch (value) {
            case static_cast<int>(web::HttpVersion::DEFAULT):
            case static_cast<int>(web::HttpVersion::VERSION_1_0):
            case static_cast<int>(web::HttpVersion::VERSION_1_1):
            case static_cast<int>(web::HttpVersion::VERSION_2_0):
            case static_cast<int>(web::HttpVersion::VERSION_2TLS):
            case static_cast<int>(web::HttpVersion::VERSION_2_PRIOR_KNOWLEDGE):
            case static_cast<int>(web::HttpVersion::VERSION_3):
            case static_cast<int>(web::HttpVersion::VERSION_3ONLY):
                return static_cast<web::HttpVersion>(value);
            default: luaL_error(L, "%s: invalid HttpVersion %d", method, value);
        }
    }

    web::ProxyOpts checkProxyOpts(lua_State* L, int idx, char const* method) {
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);

        web::ProxyOpts opts;
        opts.address = fieldString(L, idx, "address", method);

        if (auto port = optNumberField(L, idx, "port", method)) {
            if (*port < 0 || *port > 65535)
                luaL_error(L, "%s expected proxy port 0..65535", method);
            opts.port = static_cast<std::uint16_t>(*port);
        }
        if (optField(L, idx, "type")) {
            lua_getfield(L, idx, "type");
            opts.type = checkProxyType(L, -1, method);
            lua_pop(L, 1);
        }
        if (auto auth = optNumberField(L, idx, "auth", method)) {
            opts.auth = static_cast<long>(*auth);
        }
        if (auto username = optStringField(L, idx, "username", method))
            opts.username = std::move(*username);
        if (auto password = optStringField(L, idx, "password", method))
            opts.password = std::move(*password);
        if (auto tunneling = optBoolField(L, idx, "tunneling", method)) opts.tunneling = *tunneling;
        if (auto cert = optBoolField(L, idx, "certVerification", method))
            opts.certVerification = *cert;
        return opts;
    }

    void pushProgress(lua_State* L, web::WebProgress const& p) {
        lua_createtable(L, 0, 6);
        push(L, static_cast<double>(p.downloaded()));
        lua_setfield(L, -2, "downloaded");
        push(L, static_cast<double>(p.downloadTotal()));
        lua_setfield(L, -2, "downloadTotal");
        if (auto progress = p.downloadProgress()) {
            push(L, static_cast<double>(*progress));
        }
        else {
            lua_pushnil(L);
        }
        lua_setfield(L, -2, "downloadProgress");
        push(L, static_cast<double>(p.uploaded()));
        lua_setfield(L, -2, "uploaded");
        push(L, static_cast<double>(p.uploadTotal()));
        lua_setfield(L, -2, "uploadTotal");
        if (auto progress = p.uploadProgress()) {
            push(L, static_cast<double>(*progress));
        }
        else {
            lua_pushnil(L);
        }
        lua_setfield(L, -2, "uploadProgress");
    }

    void setProgressCallback(web::WebRequest& req, std::shared_ptr<LuaCallback> cb) {
        req.onProgress([cb = std::move(cb)](web::WebProgress const& progress) {
            if (!cb || !cb->valid()) return;
            geode::queueInMainThread([cb, progress] {
                if (!cb || !cb->valid()) return;
                struct Ctx {
                    web::WebProgress const* progress;
                } ctx{&progress};
                if (!cb->invoke(
                        1,
                        0,
                        "geode.utils.web.onProgress",
                        kHookScriptDeadlineMs,
                        +[](lua_State* L, void* raw) {
                            pushProgress(L, *static_cast<Ctx*>(raw)->progress);
                        },
                        &ctx
                    )) {
                    logCallbackFailure("geode.utils.web.onProgress");
                }
            });
        });
    }

    void applyOptions(lua_State* L, web::WebRequest& req, int idx, char const* method) {
        if (idx <= 0 || lua_gettop(L) < idx || lua_isnil(L, idx)) return;
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);

        applyScalarOptions(L, req, idx, method);

        if (auto value = optStringField(L, idx, "body", method)) {
            if (!applyBody(req, *value)) luaL_error(L, "%s", kWebRequestBodyExceededMsg);
        }
        if (auto value = optStringField(L, idx, "bodyString", method)) {
            if (!applyBodyString(req, *value)) luaL_error(L, "%s", kWebRequestBodyExceededMsg);
        }

        lua_getfield(L, idx, "bodyJson");
        if (!lua_isnil(L, -1)) {
            auto valueResult = toJson(L, -1, 0);
            if (valueResult.isErr()) {
                luaL_error(L, "%s", valueResult.unwrapErr().c_str());
            }
            if (!applyBodyJson(req, std::move(valueResult.unwrap()))) {
                luaL_error(L, "%s", kWebRequestBodyExceededMsg);
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "bodyMultipart");
        if (!lua_isnil(L, -1)) {
            auto& form = checkMultipartBox(L, -1, method)->form;
            if (!applyBodyMultipart(req, form)) luaL_error(L, "%s", kWebRequestBodyExceededMsg);
        }
        lua_pop(L, 1);

        lua_getfield(L, idx, "onProgress");
        if (!lua_isnil(L, -1)) {
            luaL_checktype(L, -1, LUA_TFUNCTION);
            setProgressCallback(req, std::make_shared<LuaCallback>(L, -1));
        }
        lua_pop(L, 1);
    }

    int multipartParam(lua_State* L) {
        auto* box = checkMultipartBox(L, 1, "MultipartForm:param");
        auto name = check<std::string>(L, 2, "MultipartForm:param");
        auto value = check<std::string>(L, 3, "MultipartForm:param");
        box->form.param(std::move(name), std::move(value));
        if (!requestBodyWithinLimit(box->form.getBody().size())) {
            return pushRequestBodyExceeded(L);
        }
        lua_pushvalue(L, 1);
        return 1;
    }

    int multipartFile(lua_State* L) {
        auto* box = checkMultipartBox(L, 1, "MultipartForm:file");
        auto name = check<std::string>(L, 2, "MultipartForm:file");
        auto data = check<std::string>(L, 3, "MultipartForm:file");
        if (!requestBodyWithinLimit(data.size())) {
            return pushRequestBodyExceeded(L);
        }
        auto filename = check<std::string>(L, 4, "MultipartForm:file");
        auto mime = lua_gettop(L) >= 5 && !lua_isnil(L, 5) ?
            check<std::string>(L, 5, "MultipartForm:file") :
            std::string("application/octet-stream");
        auto bytes =
            std::span<uint8_t const>(reinterpret_cast<uint8_t const*>(data.data()), data.size());
        box->form.file(std::move(name), bytes, std::move(filename), std::move(mime));
        if (!requestBodyWithinLimit(box->form.getBody().size())) {
            return pushRequestBodyExceeded(L);
        }
        lua_pushvalue(L, 1);
        return 1;
    }

    int multipartFileFrom(lua_State* L) {
        auto* box = checkMultipartBox(L, 1, "MultipartForm:fileFrom");
        auto name = check<std::string>(L, 2, "MultipartForm:fileFrom");
        auto target = resolveSandboxTarget(L, 3, 4, "MultipartForm:fileFrom", false);
        if (!target) return 2;

        std::error_code ec;
        auto fileSize = std::filesystem::file_size(target->path, ec);
        if (!ec && !requestBodyWithinLimit(fileSize)) {
            return pushRequestBodyExceeded(L);
        }

        auto mime = lua_gettop(L) >= 5 && !lua_isnil(L, 5) ?
            check<std::string>(L, 5, "MultipartForm:fileFrom") :
            std::string("application/octet-stream");
        auto result = box->form.file(std::move(name), target->path, std::move(mime));
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }
        if (!requestBodyWithinLimit(box->form.getBody().size())) {
            return pushRequestBodyExceeded(L);
        }
        lua_pushvalue(L, 1);
        return 1;
    }

    int multipartGetBoundary(lua_State* L) {
        push(L, checkMultipartBox(L, 1, "MultipartForm:getBoundary")->form.getBoundary());
        return 1;
    }

    int multipartGetHeader(lua_State* L) {
        push(L, checkMultipartBox(L, 1, "MultipartForm:getHeader")->form.getHeader());
        return 1;
    }

    int multipartGetBody(lua_State* L) {
        auto body = checkMultipartBox(L, 1, "MultipartForm:getBody")->form.getBody();
        if (!requestBodyWithinLimit(body.size())) {
            return pushRequestBodyExceeded(L);
        }
        lua_pushlstring(L, reinterpret_cast<char const*>(body.data()), body.size());
        return 1;
    }

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
        registerTaggedMetatable(L, kRequestMeta, std::nullopt, requestMethods, &requestGc);

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
        registerTaggedMetatable(L, kResponseMeta, std::nullopt, responseMethods, &responseGc);

        luaL_Reg multipartMethods[] = {
            {"param", multipartParam},
            {"file", multipartFile},
            {"fileFrom", multipartFileFrom},
            {"getBoundary", multipartGetBoundary},
            {"getHeader", multipartGetHeader},
            {"getBody", multipartGetBody},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(L, kMultipartMeta, std::nullopt, multipartMethods, &multipartGc);

        luaL_Reg handleMethods[] = {
            {"cancel", handleCancel},
            {"id", handleId},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(L, kHandleMeta, std::nullopt, handleMethods, &handleGc);

        luaL_Reg listenerMethods[] = {
            {"disconnect", listenerDisconnect},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(L, kListenerMeta, std::nullopt, listenerMethods, &listenerGc);
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
