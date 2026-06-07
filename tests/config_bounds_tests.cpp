#include "lua/Config.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("resource byte caps stay aligned across bindings") {
    REQUIRE(luax::kMaxWebResponseBytes == luax::kMaxFsReadBytes);
    REQUIRE(luax::kMaxWebResponseBytes == 32 * 1024 * 1024);
}
