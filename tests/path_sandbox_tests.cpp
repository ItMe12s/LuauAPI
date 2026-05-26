#include "lua/PathSandbox.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
    std::filesystem::path makeTempDir() {
        auto dir = std::filesystem::temp_directory_path()
            / ("luauapi_path_sandbox_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }

    void writeFile(std::filesystem::path const& path, std::string const& contents) {
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.good());
        out << contents;
        REQUIRE(out.good());
    }

    void writeOversizedFile(std::filesystem::path const& path) {
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.good());

        std::string block(1024, 'x');
        for (std::size_t written = 0; written <= luax::kMaxScriptBytes; written += block.size()) {
            out.write(block.data(), static_cast<std::streamsize>(block.size()));
            REQUIRE(out.good());
        }
    }
}

TEST_CASE("virtual chunk paths normalize to flat luau resources") {
    auto withAt = luax::normalizeVirtualPath("@Bootstrap");
    REQUIRE(withAt.isOk());
    REQUIRE(withAt.unwrap() == std::filesystem::path("Bootstrap.luau"));

    auto withExtension = luax::normalizeVirtualPath("Bootstrap.luau");
    REQUIRE(withExtension.isOk());
    REQUIRE(withExtension.unwrap() == std::filesystem::path("Bootstrap.luau"));
}

TEST_CASE("virtual chunk paths reject unsafe or unsupported names") {
    REQUIRE(luax::normalizeVirtualPath("").isErr());
    REQUIRE(luax::normalizeVirtualPath("@").isErr());
    REQUIRE(luax::normalizeVirtualPath("../Bootstrap").isErr());
    REQUIRE(luax::normalizeVirtualPath("scripts/Bootstrap").isErr());
    REQUIRE(luax::normalizeVirtualPath("Bootstrap.lua").isErr());

    auto absolute = luax::normalizedPathString(std::filesystem::temp_directory_path() / "Bootstrap.luau");
    REQUIRE(luax::normalizeVirtualPath(absolute).isErr());
}

TEST_CASE("canonical root accepts directories and rejects empty roots") {
    auto dir = makeTempDir();

    auto root = luax::canonicalRoot(dir);
    REQUIRE(root.isOk());
    REQUIRE(std::filesystem::is_directory(root.unwrap()));

    REQUIRE(luax::canonicalRoot({}).isErr());

    std::filesystem::remove_all(dir);
}

TEST_CASE("script path resolution rejects root escapes") {
    auto dir = makeTempDir();
    auto outsideDir = makeTempDir();
    auto insideFile = dir / "Inside.luau";
    auto outsideFile = outsideDir / "Outside.luau";
    auto linkFile = dir / "Link.luau";

    writeFile(insideFile, "return 1");
    writeFile(outsideFile, "return 2");

    auto root = luax::canonicalRoot(dir);
    REQUIRE(root.isOk());
    auto canonicalRoot = root.unwrap();

    auto inside = luax::resolveScriptFileInsideRoot(canonicalRoot, insideFile);
    REQUIRE(inside.isOk());

    std::error_code ec;
    std::filesystem::create_symlink(outsideFile, linkFile, ec);
    if (!ec) {
        REQUIRE(luax::resolveScriptFileInsideRoot(canonicalRoot, linkFile).isErr());
    }

    std::filesystem::remove_all(dir);
    std::filesystem::remove_all(outsideDir);
}

TEST_CASE("script file read enforces maximum source size") {
    auto dir = makeTempDir();
    auto smallFile = dir / "Small.luau";
    auto large = dir / "Large.luau";

    writeFile(smallFile, "return 1");
    writeOversizedFile(large);

    auto smallRead = luax::readScriptFile(smallFile);
    REQUIRE(smallRead.isOk());
    REQUIRE(smallRead.unwrap() == "return 1");

    REQUIRE(luax::readScriptFile(large).isErr());

    std::filesystem::remove_all(dir);
}
