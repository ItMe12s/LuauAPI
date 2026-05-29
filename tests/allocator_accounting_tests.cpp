#include "lua/runtime/AllocatorAccounting.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("allocator accounting permits growth within limit") {
    REQUIRE(luax::allocatorCanReallocate(16, 32, 8, 16));
    REQUIRE(luax::allocatorUsageAfterReallocate(16, 8, 16) == 24);
}

TEST_CASE("allocator accounting rejects growth past limit") {
    REQUIRE_FALSE(luax::allocatorCanReallocate(24, 31, 8, 16));
}

TEST_CASE("allocator accounting handles shrink and free") {
    REQUIRE(luax::allocatorCanReallocate(32, 32, 16, 8));
    REQUIRE(luax::allocatorUsageAfterReallocate(32, 16, 8) == 24);
    REQUIRE(luax::allocatorUsageAfterFree(24, 8) == 16);
}

TEST_CASE("allocator accounting clamps underflow") {
    REQUIRE(luax::allocatorUsageAfterFree(4, 8) == 0);
    REQUIRE(luax::allocatorUsageAfterReallocate(4, 8, 16) == 16);
}

TEST_CASE("allocator accounting at exact boundary") {
    REQUIRE(luax::allocatorCanReallocate(24, 32, 8, 16));
    REQUIRE_FALSE(luax::allocatorCanReallocate(24, 32, 8, 17));
    REQUIRE(luax::allocatorUsageAfterReallocate(24, 8, 16) == 32);
}
