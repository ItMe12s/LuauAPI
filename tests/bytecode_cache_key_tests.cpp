#include "host/lua_test_helpers.hpp"
#include "require/BytecodeCacheKey.hpp"
#include "require/PathSandbox.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
    void writeFile(std::filesystem::path const& path, std::string const& contents) {
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.good());
        out << contents;
        REQUIRE(out.good());
    }

    bool contains(std::string const& text, std::string const& needle) {
        return text.find(needle) != std::string::npos;
    }
} // namespace

TEST_CASE("file cache key includes path and metadata tokens") {
    auto dir = luauapi_test::ScopedTempDir{"luauapi_bytecode_cache_key_"};
    auto file = dir.path / "Module.luau";
    writeFile(file, "return 1");

    auto key = luax::fileCacheKey(file, "return 1");

    REQUIRE(contains(key, luax::filesystemPathString(file)));
    REQUIRE(contains(key, "|size="));
    REQUIRE(contains(key, "|mtime="));
    REQUIRE(contains(key, "|hash="));
}

TEST_CASE("file cache key changes when contents change") {
    auto dir = luauapi_test::ScopedTempDir{"luauapi_bytecode_cache_key_"};
    auto file = dir.path / "Module.luau";
    writeFile(file, "return 1");

    auto first = luax::fileCacheKey(file, "return 1");
    auto second = luax::fileCacheKey(file, "return 2");

    REQUIRE(first != second);
}

TEST_CASE("run script bytecode key uses filesystem root text") {
    auto dir = luauapi_test::ScopedTempDir{"luauapi_bytecode_cache_key_"};
    auto key = luax::runScriptBytecodeKey(dir.path, "@Bootstrap.luau", "return 1");

    REQUIRE(contains(key, luax::filesystemPathString(dir.path)));
    REQUIRE(contains(key, "@Bootstrap.luau"));
}

TEST_CASE("run script bytecode key omits empty resources root") {
    auto key = luax::runScriptBytecodeKey({}, "@Bootstrap.luau", "return 1");
    REQUIRE(key.starts_with("@Bootstrap.luau|"));
    REQUIRE(key.rfind('|') == key.find('|'));
}

TEST_CASE("file cache key changes when file mtime changes") {
    auto dir = luauapi_test::ScopedTempDir{"luauapi_bytecode_cache_key_"};
    auto file = dir.path / "Module.luau";
    writeFile(file, "return 1");

    auto first = luax::fileCacheKey(file, "return 1");

    auto bumped = std::filesystem::last_write_time(file) + std::chrono::hours(1);
    std::filesystem::last_write_time(file, bumped);

    auto second = luax::fileCacheKey(file, "return 1");

    REQUIRE(first != second);
}
