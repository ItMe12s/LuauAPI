#include "framework/stack/Stack.hpp"
#include "bindings/geode/web/GeodeWebInternal.hpp"
#include "bindings/geode/JsonConvert.hpp"
#include "bindings/geode/ModSandbox.hpp"
#include "bindings/geode/web/WebCaps.hpp"

#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <string_view>

namespace luax::webdetail {
    using namespace luax;

    namespace {
        int responseBool(lua_State* L, bool (web::WebResponse::*fn)() const, char const* method) {
            push(L, (checkResponse(L, 1, method).*fn)());
            return 1;
        }

        void setDurationField(lua_State* L, char const* name, auto const& duration) {
            push(L, static_cast<double>(duration.template millis<double>()));
            lua_setfield(L, -2, name);
        }
    } // namespace

    int responseInfo(lua_State* L) {
        return responseBool(L, &web::WebResponse::info, "WebResponse:info");
    }

    int responseOk(lua_State* L) {
        return responseBool(L, &web::WebResponse::ok, "WebResponse:ok");
    }

    int responseRedirected(lua_State* L) {
        return responseBool(L, &web::WebResponse::redirected, "WebResponse:redirected");
    }

    int responseBadClient(lua_State* L) {
        return responseBool(L, &web::WebResponse::badClient, "WebResponse:badClient");
    }

    int responseBadServer(lua_State* L) {
        return responseBool(L, &web::WebResponse::badServer, "WebResponse:badServer");
    }

    int responseError(lua_State* L) {
        return responseBool(L, &web::WebResponse::error, "WebResponse:error");
    }

    int responseCancelled(lua_State* L) {
        return responseBool(L, &web::WebResponse::cancelled, "WebResponse:cancelled");
    }

    int responseCode(lua_State* L) {
        push(L, checkResponse(L, 1, "WebResponse:code").code());
        return 1;
    }

    int responseText(lua_State* L) {
        auto result = checkResponse(L, 1, "WebResponse:text").string();
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }
        auto text = std::move(result.unwrap());
        if (!responseDataWithinLimit(text.size())) {
            return pushResponseSizeExceeded(L);
        }
        push(L, text);
        return 1;
    }

    int responseJson(lua_State* L) {
        auto& response = checkResponse(L, 1, "WebResponse:json");
        if (!responseDataWithinLimit(response.data().size())) {
            return pushResponseSizeExceeded(L);
        }
        auto result = response.json();
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }
        if (auto pushed = pushJson(L, result.unwrap(), 0); pushed.isErr()) {
            return pushNilErr(L, pushed.unwrapErr());
        }
        return 1;
    }

    int responseBytes(lua_State* L) {
        auto const& data = checkResponse(L, 1, "WebResponse:bytes").data();
        if (!responseDataWithinLimit(data.size())) {
            return pushResponseSizeExceeded(L);
        }
        lua_pushlstring(L, reinterpret_cast<char const*>(data.data()), data.size());
        return 1;
    }

    int responseSaveTo(lua_State* L) {
        auto& response = checkResponse(L, 1, "WebResponse:saveTo");
        if (!responseDataWithinLimit(response.data().size())) {
            return pushResponseSizeExceeded(L);
        }
        auto target = resolveSandboxTarget(L, 2, 3, "WebResponse:saveTo", true);
        if (!target) return 2;

        auto parent = target->path.parent_path();
        if (!parent.empty()) {
            auto made = geode::utils::file::createDirectoryAll(parent);
            if (made.isErr()) {
                return pushNilErr(L, made.unwrapErr());
            }
        }
        auto result = response.into(target->path);
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }
        push(L, true);
        return 1;
    }

    int responseHeaders(lua_State* L) {
        auto headers = checkResponse(L, 1, "WebResponse:headers").headers();
        lua_createtable(L, static_cast<int>(headers.size()), 0);
        int i = 1;
        for (auto const& header : headers) {
            push(L, header);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    int responseHeader(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebResponse:header");
        auto header = checkResponse(L, 1, "WebResponse:header").header(name);
        if (!header) {
            lua_pushnil(L);
            return 1;
        }
        push(L, std::string_view(*header));
        return 1;
    }

    int responseHeadersNamed(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebResponse:headersNamed");
        auto headers = checkResponse(L, 1, "WebResponse:headersNamed").getAllHeadersNamed(name);
        if (!headers) {
            lua_pushnil(L);
            return 1;
        }
        lua_createtable(L, static_cast<int>(headers->size()), 0);
        int i = 1;
        for (auto const& header : *headers) {
            push(L, header);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    int responseErrorMessage(lua_State* L) {
        push(L, checkResponse(L, 1, "WebResponse:errorMessage").errorMessage());
        return 1;
    }

    int responseVerboseLogs(lua_State* L) {
        push(L, checkResponse(L, 1, "WebResponse:verboseLogs").verboseLogs());
        return 1;
    }

    int responseTimings(lua_State* L) {
        auto const& timings = checkResponse(L, 1, "WebResponse:timings").timings();
        lua_createtable(L, 0, 8);
        setDurationField(L, "queueWait", timings.queueWait);
        setDurationField(L, "nameLookup", timings.nameLookup);
        setDurationField(L, "connect", timings.connect);
        setDurationField(L, "tlsHandshake", timings.tlsHandshake);
        setDurationField(L, "requestSend", timings.requestSend);
        setDurationField(L, "firstByte", timings.firstByte);
        setDurationField(L, "download", timings.download);
        setDurationField(L, "total", timings.total);
        return 1;
    }
} // namespace luax::webdetail
