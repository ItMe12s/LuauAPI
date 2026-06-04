#include "lua/module/PathSandbox.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
    std::filesystem::path makeTempDir() {
        auto dir = std::filesystem::temp_directory_path() /
            ("luauapi_fs_path_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }
} // namespace

TEST_CASE("fs path resolution accepts paths inside the root") {
    auto dir = makeTempDir();

    auto flat = luax::resolveInsideRoot(dir, "data.json");
    REQUIRE(flat.isOk());
    REQUIRE(luax::pathInsideRoot(flat.unwrap(), luax::canonicalRoot(dir).unwrap()));

    auto nested = luax::resolveInsideRoot(dir, "sub/dir/data.json");
    REQUIRE(nested.isOk());
    REQUIRE(luax::pathInsideRoot(nested.unwrap(), luax::canonicalRoot(dir).unwrap()));

    auto normalized = luax::resolveInsideRoot(dir, "sub/../data.json");
    REQUIRE(normalized.isOk());
    REQUIRE(normalized.unwrap() == luax::canonicalRoot(dir).unwrap() / "data.json");

    std::filesystem::remove_all(dir);
}

TEST_CASE("fs path resolution rejects traversal and absolute paths") {
    auto dir = makeTempDir();

    REQUIRE(luax::resolveInsideRoot(dir, "../escape").isErr());
    REQUIRE(luax::resolveInsideRoot(dir, "../../etc/passwd").isErr());
    REQUIRE(luax::resolveInsideRoot(dir, "sub/../../escape").isErr());
    REQUIRE(luax::resolveInsideRoot(dir, "").isErr());

    auto absolute =
        luax::normalizedPathString(std::filesystem::temp_directory_path() / "outside.json");
    REQUIRE(luax::resolveInsideRoot(dir, absolute).isErr());

    std::filesystem::remove_all(dir);
}

TEST_CASE("fs path resolution rejects a non-existent root") {
    auto missing = std::filesystem::temp_directory_path() / "luauapi_fs_path_missing_root_xyz";
    std::filesystem::remove_all(missing);
    REQUIRE(luax::resolveInsideRoot(missing, "data.json").isErr());
}

TEST_CASE("fs path resolution reports canonicalization failures") {
    auto dir = makeTempDir();
    auto file = dir / "not_a_directory.txt";
    {
        std::ofstream out(file);
        REQUIRE(out.good());
        out << "root";
    }

    auto failed = luax::resolveInsideRoot(file, "data.json");
    REQUIRE(failed.isErr());
    REQUIRE(failed.unwrapErr().find("directory") != std::string::npos);

    std::filesystem::remove_all(dir);
}
