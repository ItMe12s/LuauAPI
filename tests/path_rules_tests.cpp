#include "lua/module/PathRules.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <string>

TEST_CASE("flat resource paths accept single names") {
    REQUIRE(luax::isFlatResourcePathValue("Bootstrap"));
    REQUIRE(luax::isFlatResourcePathValue("Bootstrap.luau"));
}

TEST_CASE("flat resource paths reject traversal and directories") {
    REQUIRE_FALSE(luax::isFlatResourcePathValue(".."));
    REQUIRE_FALSE(luax::isFlatResourcePathValue("../Bootstrap"));
    REQUIRE_FALSE(luax::isFlatResourcePathValue("mod/Bootstrap"));
    REQUIRE_FALSE(luax::isFlatResourcePathValue(""));
}

TEST_CASE("flat resource paths reject absolute paths") {
    REQUIRE_FALSE(luax::isFlatResourcePathValue(std::filesystem::path("/tmp/Bootstrap.luau")));
    REQUIRE_FALSE(luax::isFlatResourcePathValue(std::filesystem::path("C:/tmp/Bootstrap.luau")));
}

TEST_CASE("extension checks only allow luau or empty extension") {
    REQUIRE(luax::hasLuauExtensionValue("Bootstrap.luau"));
    REQUIRE_FALSE(luax::hasUnsupportedExtensionValue("Bootstrap"));
    REQUIRE_FALSE(luax::hasUnsupportedExtensionValue("Bootstrap.luau"));
    REQUIRE(luax::hasUnsupportedExtensionValue("Bootstrap.lua"));
}

TEST_CASE("root escape text check catches parent traversal") {
    REQUIRE(luax::escapedRelativePathText(".."));
    REQUIRE(luax::escapedRelativePathText("../Bootstrap"));
    REQUIRE(luax::escapedRelativePathText("..\\Bootstrap"));
    REQUIRE_FALSE(luax::escapedRelativePathText("Bootstrap"));
}

TEST_CASE("root containment rejects paths outside root") {
    auto base = std::filesystem::temp_directory_path()
        / ("luauapi_path_rules_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto root = base / "root";
    auto child = root / "Bootstrap.luau";
    auto outside = base / "outside" / "Bootstrap.luau";

    REQUIRE(std::filesystem::create_directories(root));
    REQUIRE(std::filesystem::create_directories(outside.parent_path()));

    REQUIRE(luax::pathInsideRootValue(child, root));
    REQUIRE_FALSE(luax::pathInsideRootValue(outside, root));

    std::filesystem::remove_all(base);
}
