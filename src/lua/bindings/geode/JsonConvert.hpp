#pragma once

#include "lua/Config.hpp"

#include <lua.h>
#include <matjson.hpp>

namespace luax {
    void pushJson(lua_State* L, matjson::Value const& value, int depth = 0);

    matjson::Value toJson(lua_State* L, int idx, int depth = 0);
} // namespace luax
