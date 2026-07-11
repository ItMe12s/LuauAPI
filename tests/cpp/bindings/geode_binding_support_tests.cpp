#include "bindings/geode/GeodeBindingSupport.hpp"
#include "host/lua_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>
#include <utility>

namespace {
    int checkColor(lua_State* L) {
        auto color = luax::geode_detail::checkColorChannels(L, 1, "test.color");
        for (auto channel : color)
            lua_pushinteger(L, channel);
        return 4;
    }

    int checkDenseArray(lua_State* L) {
        lua_pushinteger(
            L, static_cast<lua_Integer>(luax::geode_detail::checkDenseArray(L, 1, "test.choice"))
        );
        return 1;
    }

    struct Counted {
        int* destructions = nullptr;

        explicit Counted(int* count) : destructions(count) {}

        Counted(Counted const&) = delete;
        Counted& operator=(Counted const&) = delete;

        Counted(Counted&& other) noexcept :
            destructions(std::exchange(other.destructions, nullptr)) {}

        Counted& operator=(Counted&&) = delete;

        ~Counted() {
            if (destructions) ++*destructions;
        }
    };
} // namespace

TEST_CASE("Geode color facade validates byte channels") {
    auto guard = luauapi_test::makeLuaState(true);
    auto* L = guard.get();
    lua_pushcfunction(L, &checkColor, "test.color");
    lua_setglobal(L, "checkColor");

    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L,
            "local r, g, b, a = checkColor({ r = 1, g = 2, b = 3, a = 255 }) "
            "return r == 1 and g == 2 and b == 3 and a == 255"
        )
    );
    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L, "return not pcall(checkColor, { r = 256, g = 2, b = 3, a = 255 })"
        )
    );
    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L, "return not pcall(checkColor, { r = 1.5, g = 2, b = 3, a = 255 })"
        )
    );
    REQUIRE(
        luauapi_test::runScriptReturnsBool(L, "return not pcall(checkColor, { r = 1, g = 2, b = 3 })")
    );
}

TEST_CASE("Geode random choice accepts only dense arrays") {
    auto guard = luauapi_test::makeLuaState(true);
    auto* L = guard.get();
    lua_pushcfunction(L, &checkDenseArray, "test.choice");
    lua_setglobal(L, "checkDenseArray");

    REQUIRE(luauapi_test::runScriptReturnsBool(L, "return checkDenseArray({ 'a', 'b' }) == 2"));
    REQUIRE(luauapi_test::runScriptReturnsBool(L, "return not pcall(checkDenseArray, {})"));
    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L, "return not pcall(checkDenseArray, { [1] = 'a', [3] = 'c' })"
        )
    );
    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L, "return not pcall(checkDenseArray, { 'a', extra = true, [4] = 'd' })"
        )
    );
}

TEST_CASE("Geode owned userdata runs its native destructor") {
    auto guard = luauapi_test::makeLuaState();
    auto* L = guard.get();
    constexpr char const* meta = "luax.test.GeodeBindingOwned";
    constexpr int tag = 200;
    REQUIRE(luaL_newmetatable(L, meta) != 0);
    lua_pop(L, 1);
    luaL_getmetatable(L, meta);
    lua_setuserdatametatable(L, tag);
    lua_setuserdatadtor(L, tag, &luax::geode_detail::destroyOwned<Counted>);

    int destructions = 0;
    luax::geode_detail::pushOwned(L, tag, Counted(&destructions));
    lua_pop(L, 1);
    luauapi_test::collectGarbage(L);

    REQUIRE(destructions == 1);
}
