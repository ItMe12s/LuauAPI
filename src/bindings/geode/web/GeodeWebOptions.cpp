#include "framework/callback/LuaCallback.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "bindings/geode/web/GeodeWebInternal.hpp"
#include "bindings/geode/JsonConvert.hpp"
#include "bindings/geode/web/WebCaps.hpp"

#include <Geode/utils/web.hpp>
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
} // namespace luax::webdetail
