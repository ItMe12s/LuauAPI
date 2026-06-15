#include "core/Loadstring.hpp"
#include "host/lua_test_helpers.hpp"
#include "require/BytecodeCacheKey.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <string>

using luauapi_test::compile;
using luauapi_test::makeLuaState;

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