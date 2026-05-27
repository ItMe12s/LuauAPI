#include "lua/bindings/internal/Usertype.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <typeindex>
#include <vector>

namespace {
    template <int N>
    struct FakeType {};

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
