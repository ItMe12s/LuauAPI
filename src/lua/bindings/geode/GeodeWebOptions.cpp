#include "lua/bindings/geode/GeodeWebInternal.hpp"
#include "lua/bindings/framework/LuaCallback.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/JsonConvert.hpp"
#include "lua/bindings/geode/WebCaps.hpp"

#include <Geode/utils/web.hpp>
#include <chrono>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace luax::webdetail {
    using namespace luax;

    namespace {
        web::ProxyType checkProxyType(lua_State* L, int idx, char const* method) {
            int value = check<int>(L, idx, method);
            if (value < static_cast<int>(web::ProxyType::HTTP) ||
                value > static_cast<int>(web::ProxyType::SOCKS5H)) {
                luaL_error(L, "%s: invalid ProxyType %d", method, value);
            }
            return static_cast<web::ProxyType>(value);
        }

        void applyStringMap(
            lua_State* L, int idx, char const* field, char const* method, auto&& setter
        ) {
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

    int optPriority(lua_State* L, int idx) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return geode::Priority::Normal;
        return check<int>(L, idx, "geode.utils.web listener");
    }

    void logWebCallbackFailure(char const* context) {
        geode::log::warn("[lua:{}] callback failed", context);
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
                    logWebCallbackFailure("geode.utils.web.onProgress");
                }
            });
        });
    }

    void applyOptions(lua_State* L, web::WebRequest& req, int idx, char const* method) {
        if (idx <= 0 || lua_gettop(L) < idx || lua_isnil(L, idx)) return;
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);

        applyStringMap(L, idx, "headers", method, [&](std::string name, std::string value) {
            req.header(std::move(name), std::move(value));
        });
        applyStringMap(L, idx, "params", method, [&](std::string name, std::string value) {
            req.param(std::move(name), std::move(value));
        });

        if (auto value = optStringField(L, idx, "method", method)) req.method(std::move(*value));
        if (auto value = optStringField(L, idx, "url", method)) req.url(std::move(*value));
        if (auto value = optStringField(L, idx, "userAgent", method))
            req.userAgent(std::move(*value));
        if (auto value = optStringField(L, idx, "acceptEncoding", method))
            req.acceptEncoding(std::move(*value));
        if (auto value = optNumberField(L, idx, "timeout", method)) {
            if (*value < 0) luaL_error(L, "%s expected timeout >= 0", method);
            req.timeout(std::chrono::seconds(static_cast<std::int64_t>(*value)));
        }
        if (auto value = optDownloadRange(L, idx, method)) req.downloadRange(*value);
        if (auto value = optBoolField(L, idx, "certVerification", method))
            req.certVerification(*value);
        if (auto value = optBoolField(L, idx, "transferBody", method)) req.transferBody(*value);
        if (auto value = optBoolField(L, idx, "followRedirects", method))
            req.followRedirects(*value);
        if (auto value = optBoolField(L, idx, "ignoreContentLength", method))
            req.ignoreContentLength(*value);
        if (auto value = optStringField(L, idx, "caBundle", method))
            req.CABundleContent(std::move(*value));

        lua_getfield(L, idx, "proxy");
        if (!lua_isnil(L, -1)) req.proxyOpts(checkProxyOpts(L, -1, method));
        lua_pop(L, 1);

        lua_getfield(L, idx, "version");
        if (!lua_isnil(L, -1)) req.version(checkHttpVersion(L, -1, method));
        lua_pop(L, 1);

        if (auto value = optStringField(L, idx, "body", method)) {
            if (!requestBodyWithinLimit(value->size())) {
                luaL_error(L, "%s", kWebRequestBodyExceededMsg);
            }
            geode::ByteVector bytes(value->begin(), value->end());
            req.body(std::move(bytes));
        }
        if (auto value = optStringField(L, idx, "bodyString", method)) {
            if (!requestBodyWithinLimit(value->size())) {
                luaL_error(L, "%s", kWebRequestBodyExceededMsg);
            }
            req.bodyString(*value);
        }

        lua_getfield(L, idx, "bodyJson");
        if (!lua_isnil(L, -1)) req.bodyJSON(toJson(L, -1, 0));
        lua_pop(L, 1);

        lua_getfield(L, idx, "bodyMultipart");
        if (!lua_isnil(L, -1)) req.bodyMultipart(checkMultipartBox(L, -1, method)->form);
        lua_pop(L, 1);

        lua_getfield(L, idx, "onProgress");
        if (!lua_isnil(L, -1)) {
            luaL_checktype(L, -1, LUA_TFUNCTION);
            setProgressCallback(req, std::make_shared<LuaCallback>(L, -1));
        }
        lua_pop(L, 1);
    }
} // namespace luax::webdetail
