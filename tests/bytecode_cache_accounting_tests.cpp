#include "core/BytecodeCacheAccounting.hpp"

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

TEST_CASE("bytecode cache accounting enforces compile time budget") {
    REQUIRE(luax::compileTimeWithinBudget(0, 5000));
    REQUIRE(luax::compileTimeWithinBudget(5000, 5000));
    REQUIRE_FALSE(luax::compileTimeWithinBudget(5001, 5000));
}

TEST_CASE("bytecode cache accounting checks unified memory budget") {
    REQUIRE(luax::memoryBudgetAllows(100, 512, 400));
    REQUIRE_FALSE(luax::memoryBudgetAllows(100, 512, 413));
    REQUIRE(luax::memoryBudgetAllows(0, 512, 512));
}

TEST_CASE("bytecode cache insert eviction combines cache and memory limits") {
    REQUIRE(luax::bytecodeCacheInsertNeedsEviction(90, 100, 20, 1, 512, 0, 512));
    REQUIRE_FALSE(luax::bytecodeCacheInsertNeedsEviction(80, 100, 20, 1, 512, 0, 512));
    REQUIRE(luax::bytecodeCacheInsertNeedsEviction(10, 100, 20, 1, 512, 500, 512));
    REQUIRE_FALSE(luax::bytecodeCacheInsertNeedsEviction(10, 100, 20, 1, 512, 100, 512));
    REQUIRE_FALSE(luax::bytecodeCacheInsertNeedsEviction(0, 100, 20, 0, 512, 500, 512));
}
