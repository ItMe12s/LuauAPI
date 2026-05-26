#include "lua/RequirePath.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("require only starts from chunk names") {
    REQUIRE(luax::canRequireFromChunk("@Bootstrap.luau"));
    REQUIRE_FALSE(luax::canRequireFromChunk("Bootstrap.luau"));
    REQUIRE_FALSE(luax::canRequireFromChunk(""));
}

TEST_CASE("require child names stay flat") {
    REQUIRE(luax::isRequireChildNameAllowed("Module"));
    REQUIRE(luax::isRequireChildNameAllowed("Module.luau"));
    REQUIRE_FALSE(luax::isRequireChildNameAllowed("Module.lua"));
    REQUIRE_FALSE(luax::isRequireChildNameAllowed(""));
    REQUIRE_FALSE(luax::isRequireChildNameAllowed(".."));
    REQUIRE_FALSE(luax::isRequireChildNameAllowed("../Module"));
    REQUIRE_FALSE(luax::isRequireChildNameAllowed("mod/Module"));
    REQUIRE_FALSE(luax::isRequireChildNameAllowed("mod\\Module"));
}

TEST_CASE("require module path appends luau suffix") {
    REQUIRE(luax::requireModulePath(std::filesystem::path("Module")) == std::filesystem::path("Module.luau"));
    REQUIRE(luax::requireModulePath(std::filesystem::path("Module.luau")) == std::filesystem::path("Module.luau"));
}
