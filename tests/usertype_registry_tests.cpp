#include "lua/bindings/framework/Usertype.hpp"
#include "lua/runtime/Runtime.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>

#include <cstdint>
#include <typeindex>
#include <vector>

namespace {
    template <int N>
    struct FakeType {};

    struct FieldTestObject : cocos2d::CCObject {};

    int fieldGetter(lua_State* L) {
        lua_pushinteger(L, 7);
        return 1;
    }

    int fieldSetter(lua_State*) {
        return 0;
    }

    template <int... Ns>
    void registerMany(std::integer_sequence<int, Ns...>) {
        auto& reg = luax::detail::UsertypeRegistry::get();
        (reg.tagFor(std::type_index(typeid(FakeType<Ns>))), ...);
    }
}

TEST_CASE("usertype registry survives rehash") {
    auto& reg = luax::detail::UsertypeRegistry::get();

    registerMany(std::make_integer_sequence<int, 128>{});

    for (std::uint32_t tag = 1; tag <= 128; ++tag) {
        auto const* info = reg.findByTag(tag);
        REQUIRE(info != nullptr);
        REQUIRE(info->tag == tag);
    }
}

TEST_CASE("usertype registry tagFor is idempotent") {
    auto& reg = luax::detail::UsertypeRegistry::get();
    auto idx = std::type_index(typeid(FakeType<200>));
    std::uint32_t first = reg.tagFor(idx);
    std::uint32_t second = reg.tagFor(idx);
    REQUIRE(first == second);
    REQUIRE(first != 0);
}

TEST_CASE("usertype registry findByTag returns null for unknown tag") {
    auto const* info = luax::detail::UsertypeRegistry::get().findByTag(999999);
    REQUIRE(info == nullptr);
}

TEST_CASE("usertype field registration stores getter and setter") {
    luax::Runtime::resetForHostTests();
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    auto result = luax::Usertype<FieldTestObject>::registerType(L, "FieldTestObject");
    REQUIRE(result.isOk());
    luax::Usertype<FieldTestObject>::field(L, "m_value", &fieldGetter, &fieldSetter);

    luaL_getmetatable(L, "luax:FieldTestObject");
    REQUIRE(lua_istable(L, -1));
    lua_getfield(L, -1, "__fields");
    REQUIRE(lua_istable(L, -1));
    lua_getfield(L, -1, "m_value");
    REQUIRE(lua_istable(L, -1));
    lua_getfield(L, -1, "get");
    REQUIRE(lua_isfunction(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -1, "set");
    REQUIRE(lua_isfunction(L, -1));
    lua_pop(L, 4);

    luax::Runtime::resetForHostTests();
}
