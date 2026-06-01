#pragma once

#include <matjson.hpp>

#include <lua.h>

namespace luax {
    constexpr int kMaxJsonDepth = 32;

    void pushJson(lua_State* L, matjson::Value const& value, int depth = 0);

    matjson::Value toJson(lua_State* L, int idx, int depth = 0);
}
