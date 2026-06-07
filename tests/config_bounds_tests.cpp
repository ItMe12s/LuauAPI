#include "lua/Config.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("resource byte caps stay aligned across bindings") {
    REQUIRE(luax::kMaxWebResponseBytes == luax::kMaxFsReadBytes);
    REQUIRE(luax::kMaxWebResponseBytes == 32 * 1024 * 1024);
}

TEST_CASE("filesystem read and write caps are symmetric") {
    REQUIRE(luax::kMaxFsWriteBytes == luax::kMaxFsReadBytes);
    REQUIRE(luax::kMaxFsWriteBytes == 32 * 1024 * 1024);
}
