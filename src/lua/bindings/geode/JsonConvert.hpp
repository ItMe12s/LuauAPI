#pragma once

#include "lua/Config.hpp"

#include <Geode/Result.hpp>
#include <lua.h>
#include <matjson.hpp>

namespace luax {
    geode::Result<void> pushJson(lua_State* L, matjson::Value const& value, int depth = 0);

    geode::Result<matjson::Value> toJson(lua_State* L, int idx, int depth = 0);
} // namespace luax
