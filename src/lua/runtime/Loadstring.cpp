#include "Loadstring.hpp"

#include <lualib.h>
#include <string>

namespace luax {
    namespace {
        std::string stackValueToString(lua_State* L, int idx) {
            size_t len = 0;
            char const* text = luaL_tolstring(L, idx, &len);
            std::string out = text ? std::string(text, len) : std::string();
            lua_pop(L, 1);
            return out;
        }
    } // namespace

    LoadstringStatus loadstringPushResult(
        lua_State* L, std::string_view chunkName, std::string const& bytecode
    ) {
        std::string chunk(chunkName);
        if (luau_load(L, chunk.c_str(), bytecode.data(), bytecode.size(), 0) == 0) {
            return LoadstringStatus::Success;
        }

        auto err = stackValueToString(L, -1);
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushlstring(L, err.data(), err.size());
        return LoadstringStatus::Failure;
    }
} // namespace luax
