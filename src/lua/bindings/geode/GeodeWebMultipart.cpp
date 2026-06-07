#include "lua/bindings/geode/GeodeWebInternal.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/geode/ModSandbox.hpp"
#include "lua/bindings/geode/WebCaps.hpp"

#include <Geode/utils/web.hpp>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <span>
#include <string>
#include <system_error>
#include <utility>

namespace luax::webdetail {
    using namespace luax;

    int multipartParam(lua_State* L) {
        auto* box = checkMultipartBox(L, 1, "MultipartForm:param");
        auto name = check<std::string>(L, 2, "MultipartForm:param");
        auto value = check<std::string>(L, 3, "MultipartForm:param");
        box->form.param(std::move(name), std::move(value));
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
        if (ec || !requestBodyWithinLimit(fileSize)) {
            return pushRequestBodyExceeded(L);
        }

        auto mime = lua_gettop(L) >= 5 && !lua_isnil(L, 5) ?
            check<std::string>(L, 5, "MultipartForm:fileFrom") :
            std::string("application/octet-stream");
        auto result = box->form.file(std::move(name), target->path, std::move(mime));
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
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
} // namespace luax::webdetail
