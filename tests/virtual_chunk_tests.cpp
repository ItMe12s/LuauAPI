#include "require/VirtualChunk.hpp"

#include "bindings/geode/CurrentMod.hpp"
#include "core/Runtime.hpp"

#include <LuauAPI.hpp>

#include <Geode/loader/Mod.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <thread>

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::invalidateCurrentModCache();
            geode::Mod::resetForTests();
            luax::Runtime::resetForTests();
        }
    };

    std::filesystem::path makeTempDir() {
        auto dir = std::filesystem::temp_directory_path() /
            ("luauapi_virtual_chunk_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }
} // namespace

TEST_CASE("virtual chunk parsing strips mod prefix for script path") {
    auto parts = luax::parseVirtualChunk("@example.mod:Bootstrap.luau");
    REQUIRE(parts.modId == "example.mod");
    REQUIRE(parts.scriptPath == "Bootstrap.luau");
    REQUIRE(luax::virtualChunkScriptPath("@example.mod:Bootstrap.luau") == "Bootstrap.luau");
}

TEST_CASE("virtual chunk without mod prefix keeps script path") {
    auto parts = luax::parseVirtualChunk("@Bootstrap.luau");
    REQUIRE(parts.modId.empty());
    REQUIRE(parts.scriptPath == "Bootstrap.luau");
}

TEST_CASE("formatVirtualChunk builds mod attributed chunk names") {
    REQUIRE(luax::formatVirtualChunk("example.mod", "Bootstrap.luau") == "@example.mod:Bootstrap.luau");
}

TEST_CASE("modIdForResourcesRoot resolves registered mod") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir, "example.mod");

    auto modId = luax::modIdForResourcesRoot(dir);
    REQUIRE(modId.has_value());
    REQUIRE(*modId == "example.mod");
    REQUIRE(luax::modForResourcesRoot(dir) == mod);

    std::filesystem::remove_all(dir);
}

TEST_CASE("runScript errors include mod attributed chunk names") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    geode::Mod::create(dir, "example.mod");

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);

    auto result = runtime->runScript("error('boom')", "@example.mod:Broken.luau");
    REQUIRE(result.isErr());
    REQUIRE(runtime->lastError().find("example.mod:Broken.luau") != std::string::npos);
    REQUIRE(runtime->lastError().find("boom") != std::string::npos);

    std::filesystem::remove_all(dir);
}

TEST_CASE("runFile uses mod attributed chunk names") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    geode::Mod::create(dir, "example.mod");
    {
        std::ofstream file(dir / "Broken.luau");
        file << "error('from file')";
    }

    auto result = imes::luauapi::runFile(dir, "Broken.luau");
    REQUIRE(result.isErr());
    auto err = imes::luauapi::lastError();
    REQUIRE(err.find("example.mod:Broken.luau") != std::string::npos);
    REQUIRE(err.find("from file") != std::string::npos);

    std::filesystem::remove_all(dir);
}
