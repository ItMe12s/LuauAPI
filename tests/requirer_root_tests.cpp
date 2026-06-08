#include "lua/module/Requirer.hpp"
#include "lua/runtime/Runtime.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::Runtime::resetForTests();
        }
    };

    std::filesystem::path makeTempDir() {
        auto dir = std::filesystem::temp_directory_path() /
            ("luauapi_requirer_root_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }

    void writeFile(std::filesystem::path const& path, std::string const& contents) {
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.good());
        out << contents;
        REQUIRE(out.good());
    }
} // namespace

TEST_CASE("Requirer rejects empty and unresolvable resources roots") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    luax::Requirer req(*runtime);

    req.setResourcesRoot({});
    REQUIRE(req.resourcesRoot().empty());
    auto emptyRootErr = req.resolvedModulePath();
    REQUIRE(emptyRootErr.isErr());
    REQUIRE(emptyRootErr.unwrapErr() == "resources root is empty");
    REQUIRE(req.chunkname().empty());
    REQUIRE(req.toChild("Child") == NAVIGATE_NOT_FOUND);

    auto missing = std::filesystem::temp_directory_path() / "luauapi_requirer_missing_root";
    req.setResourcesRoot(missing);
    REQUIRE(req.resourcesRoot().empty());
    auto missingRootErr = req.resolvedModulePath();
    REQUIRE(missingRootErr.isErr());
    REQUIRE(missingRootErr.unwrapErr().find("resources root is not a directory") != std::string::npos);
    REQUIRE(req.toChild("Child") == NAVIGATE_NOT_FOUND);
}

TEST_CASE("Requirer resolves modules under a valid canonical root") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    auto dir = makeTempDir();
    writeFile(dir / "Child.luau", "return 1");

    luax::Requirer req(*runtime);
    req.setResourcesRoot(dir);
    REQUIRE_FALSE(req.resourcesRoot().empty());
    REQUIRE(req.toChild("Child") == NAVIGATE_SUCCESS);

    auto resolved = req.resolvedModulePath();
    REQUIRE(resolved.isOk());
    REQUIRE(std::filesystem::is_regular_file(resolved.unwrap()));

    auto chunk = req.chunkname();
    REQUIRE_FALSE(chunk.empty());
    REQUIRE(chunk.starts_with("@"));
    REQUIRE(chunk.find("Child") != std::string::npos);

    std::filesystem::remove_all(dir);
}
