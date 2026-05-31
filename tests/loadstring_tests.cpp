#include "lua/module/BytecodeCacheKey.hpp"
#include "lua/runtime/Loadstring.hpp"

#include <Luau/Compiler.h>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>

#include <memory>
#include <string>

namespace {
    struct LuaStateDeleter {
        void operator()(lua_State* L) const {
            if (L) {
                lua_close(L);
            }
        }
    };

    using LuaStatePtr = std::unique_ptr<lua_State, LuaStateDeleter>;

    LuaStatePtr makeLuaState() {
        auto* L = luaL_newstate();
        REQUIRE(L != nullptr);
        return LuaStatePtr(L);
    }

    std::string compile(std::string const& source) {
        Luau::CompileOptions opts;
        opts.optimizationLevel = 2;
        opts.debugLevel = 1;
        opts.typeInfoLevel = 1;
        return Luau::compile(source, opts);
    }
}

TEST_CASE("loadstring result returns compiled function without running it") {
    auto L = makeLuaState();
    auto bytecode = compile("return 41");

    auto status = luax::loadstringPushResult(L.get(), "=loadstring_test", bytecode);

    REQUIRE(status == luax::LoadstringStatus::Success);
    REQUIRE(lua_isfunction(L.get(), -1));
    REQUIRE(lua_pcall(L.get(), 0, 1, 0) == 0);
    REQUIRE(lua_tonumber(L.get(), -1) == 41.0);
}

TEST_CASE("loadstring result returns nil and error string on invalid source") {
    auto L = makeLuaState();
    auto bytecode = compile("return (");

    auto status = luax::loadstringPushResult(L.get(), "CustomChunk", bytecode);

    REQUIRE(status == luax::LoadstringStatus::Failure);
    REQUIRE(lua_isnil(L.get(), -2));
    REQUIRE(lua_isstring(L.get(), -1));

    std::string err(lua_tostring(L.get(), -1));
    REQUIRE(err.find("CustomChunk") != std::string::npos);
}

TEST_CASE("loadstring bytecode key changes with source content") {
    auto first = luax::loadstringBytecodeKey("=loadstring", "return 1");
    auto second = luax::loadstringBytecodeKey("=loadstring", "return 2");

    REQUIRE(first != second);
}
