#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/GeodeWebInternal.hpp"
#include "lua/bindings/geode/WebCaps.hpp"

#include <Geode/utils/web.hpp>
#include <chrono>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <matjson.hpp>
#include <optional>
#include <string>
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
} // namespace luax::webdetail
