#include "core/Loadstring.hpp"

#include "framework/stack/Stack.hpp"

#include <lualib.h>

namespace luax {
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
