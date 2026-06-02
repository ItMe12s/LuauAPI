#include "lua/runtime/BytecodeCacheAccounting.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("bytecode cache accounting tracks insert and remove") {
    REQUIRE(luax::bytecodeEntryBytes("abc") == 3);
    REQUIRE(luax::bytecodeCacheUsageAfterInsert(10, 5) == 15);
    REQUIRE(luax::bytecodeCacheUsageAfterRemove(15, 5) == 10);
    REQUIRE(luax::bytecodeCacheUsageAfterRemove(4, 8) == 0);
}

TEST_CASE("bytecode cache accounting detects when eviction is needed") {
    REQUIRE(luax::bytecodeCacheNeedsEviction(90, 100, 20, 1, 512));
    REQUIRE_FALSE(luax::bytecodeCacheNeedsEviction(80, 100, 20, 1, 512));
    REQUIRE(luax::bytecodeCacheNeedsEviction(0, 100, 1, 512, 512));
    REQUIRE_FALSE(luax::bytecodeCacheNeedsEviction(0, 100, 200, 0, 512));
}
