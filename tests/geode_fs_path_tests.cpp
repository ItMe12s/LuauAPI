#include "host/lua_test_helpers.hpp"
#include "require/PathSandbox.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <string>

TEST_CASE("fs path resolution accepts paths inside the root") {
    luauapi_test::ScopedTempDir dir{"luauapi_fs_path_"};

    auto flat = luax::resolveInsideRoot(dir.path, "data.json");
    REQUIRE(flat.isOk());
    REQUIRE(luax::pathInsideRoot(flat.unwrap(), luax::canonicalRoot(dir.path).unwrap()));

    auto nested = luax::resolveInsideRoot(dir.path, "sub/dir/data.json");
    REQUIRE(nested.isOk());
    REQUIRE(luax::pathInsideRoot(nested.unwrap(), luax::canonicalRoot(dir.path).unwrap()));

    auto normalized = luax::resolveInsideRoot(dir.path, "sub/../data.json");
    REQUIRE(normalized.isOk());
    REQUIRE(normalized.unwrap() == luax::canonicalRoot(dir.path).unwrap() / "data.json");
}

TEST_CASE("fs path resolution rejects traversal and absolute paths") {
    luauapi_test::ScopedTempDir dir{"luauapi_fs_path_"};

    REQUIRE(luax::resolveInsideRoot(dir.path, "../escape").isErr());
    REQUIRE(luax::resolveInsideRoot(dir.path, "../../etc/passwd").isErr());
    REQUIRE(luax::resolveInsideRoot(dir.path, "sub/../../escape").isErr());
    REQUIRE(luax::resolveInsideRoot(dir.path, "").isErr());

    auto absolute =
        luax::normalizedPathString(std::filesystem::temp_directory_path() / "outside.json");
    REQUIRE(luax::resolveInsideRoot(dir.path, absolute).isErr());
}

TEST_CASE("fs path resolution rejects a non-existent root") {
    auto missing = std::filesystem::temp_directory_path() / "luauapi_fs_path_missing_root_xyz";
    std::filesystem::remove_all(missing);
    REQUIRE(luax::resolveInsideRoot(missing, "data.json").isErr());
}

TEST_CASE("fs path resolution reports canonicalization failures") {
    luauapi_test::ScopedTempDir dir{"luauapi_fs_path_"};
    auto file = dir.path / "not_a_directory.txt";
    {
        std::ofstream out(file);
        REQUIRE(out.good());
        out << "root";
    }

    auto failed = luax::resolveInsideRoot(file, "data.json");
    REQUIRE(failed.isErr());
    REQUIRE(failed.unwrapErr().find("directory") != std::string::npos);
}
