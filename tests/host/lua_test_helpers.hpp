#pragma once

#include "framework/usertype/LuaRef.hpp"
#include "core/Runtime.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>

#include <string_view>

namespace luauapi_test {
    inline void loadFunction(lua_State* L, std::string_view source, char const* chunk = "=test") {
        auto bytecode = luax::Runtime::compileSource(source);
        REQUIRE(luau_load(L, chunk, bytecode.data(), bytecode.size(), 0) == 0);
    }

    inline luax::LuaRef makeCallback(lua_State* L, std::string_view source) {
        loadFunction(L, source);
        luax::LuaRef ref(L, -1);
        lua_pop(L, 1);
        return ref;
    }
}
