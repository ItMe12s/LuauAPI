#include "lua/module/BytecodeCacheKey.hpp"
#include "lua/module/PathSandbox.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
    std::filesystem::path makeTempDir() {
        auto dir = std::filesystem::temp_directory_path()
            / ("luauapi_bytecode_cache_key_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }

    void writeFile(std::filesystem::path const& path, std::string const& contents) {
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.good());
        out << contents;
        REQUIRE(out.good());
    }

    bool contains(std::string const& text, std::string const& needle) {
        return text.find(needle) != std::string::npos;
    }
}

TEST_CASE("bytecode cache key includes path and metadata tokens") {
    auto dir = makeTempDir();
    auto file = dir / "Module.luau";
    writeFile(file, "return 1");

    auto key = luax::bytecodeCacheKey(file, "return 1");

    REQUIRE(contains(key, luax::filesystemPathString(file)));
    REQUIRE(contains(key, "|size="));
    REQUIRE(contains(key, "|mtime="));
    REQUIRE(contains(key, "|hash="));

    std::filesystem::remove_all(dir);
}

TEST_CASE("bytecode cache key changes when contents change") {
    auto dir = makeTempDir();
    auto file = dir / "Module.luau";
    writeFile(file, "return 1");

    auto first = luax::bytecodeCacheKey(file, "return 1");
    auto second = luax::bytecodeCacheKey(file, "return 2");

    REQUIRE(first != second);

    std::filesystem::remove_all(dir);
}

TEST_CASE("run script bytecode key uses filesystem root text") {
    auto dir = makeTempDir();
    auto key = luax::runScriptBytecodeKey(dir, "@Bootstrap.luau", "return 1");

    REQUIRE(contains(key, luax::filesystemPathString(dir)));
    REQUIRE(contains(key, "@Bootstrap.luau"));

    std::filesystem::remove_all(dir);
}

TEST_CASE("run script bytecode key omits empty resources root") {
    auto key = luax::runScriptBytecodeKey({}, "@Bootstrap.luau", "return 1");
    REQUIRE(key.starts_with("@Bootstrap.luau|"));
    REQUIRE(key.rfind('|') == key.find('|'));
}

TEST_CASE("bytecode cache key changes when file mtime changes") {
    auto dir = makeTempDir();
    auto file = dir / "Module.luau";
    writeFile(file, "return 1");

    auto first = luax::bytecodeCacheKey(file, "return 1");

    auto bumped = std::filesystem::last_write_time(file) + std::chrono::hours(1);
    std::filesystem::last_write_time(file, bumped);

    auto second = luax::bytecodeCacheKey(file, "return 1");

    REQUIRE(first != second);

    std::filesystem::remove_all(dir);
}
