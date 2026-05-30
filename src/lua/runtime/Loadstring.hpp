#pragma once

#include <lua.h>

#include <string>
#include <string_view>

namespace luax {
    enum class LoadstringStatus {
        Success,
        Failure,
    };

    LoadstringStatus loadstringPushResult(lua_State* L, std::string_view chunkName, std::string const& bytecode);
}
