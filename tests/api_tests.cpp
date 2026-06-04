#include "lua/runtime/Runtime.hpp"

#include <LuauAPI.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <lua.h>
#include <lualib.h>
#include <optional>
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
        auto root = std::filesystem::temp_directory_path() / "luauapi_api_tests";
        std::filesystem::create_directories(root);
        return root;
    }

    void writeScript(std::filesystem::path const& path, std::string_view source) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        REQUIRE(out.good());
        out.write(source.data(), static_cast<std::streamsize>(source.size()));
    }
} // namespace

TEST_CASE("runScript executes source from resources root") {
    RuntimeGuard guard;
    auto root = makeTempDir() / "run_script";
    std::filesystem::create_directories(root);

    auto result = imes::luauapi::runScript(root, "return 17", "script.luau");
    REQUIRE(result.isOk());
}

TEST_CASE("runScript rejects oversized source") {
    RuntimeGuard guard;
    auto root = makeTempDir() / "run_script_oversized";
    std::filesystem::create_directories(root);

    std::string source(luax::kMaxScriptBytes + 1, 'a');
    auto result = imes::luauapi::runScript(root, source, "big.luau");
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr().find("maximum size") != std::string::npos);
}

TEST_CASE("runFile loads and executes a flat script resource") {
    RuntimeGuard guard;
    auto root = makeTempDir() / "run_file";
    std::filesystem::create_directories(root);
    writeScript(root / "entry.luau", "return 42");

    auto result = imes::luauapi::runFile(root, "entry.luau");
    REQUIRE(result.isOk());
}

TEST_CASE("runFile rejects missing script") {
    RuntimeGuard guard;
    auto root = makeTempDir() / "run_file_missing";
    std::filesystem::create_directories(root);

    auto result = imes::luauapi::runFile(root, "missing.luau");
    REQUIRE(result.isErr());
}

TEST_CASE("api entry points require the main thread") {
    RuntimeGuard guard;
    auto root = makeTempDir() / "main_thread";
    std::filesystem::create_directories(root);

    std::optional<geode::Result<void>> offThreadResult;
    std::thread worker([&] {
        offThreadResult = imes::luauapi::runScript(root, "return 1", "offthread.luau");
    });
    worker.join();

    REQUIRE(offThreadResult.has_value());
    REQUIRE(offThreadResult->isErr());
    REQUIRE(offThreadResult->unwrapErr().find("main thread") != std::string::npos);
}

TEST_CASE("isReady reflects runtime initialization") {
    RuntimeGuard guard;
    REQUIRE_FALSE(imes::luauapi::isReady());

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    REQUIRE(imes::luauapi::isReady());
    REQUIRE(imes::luauapi::status() == imes::luauapi::RuntimeStatus::Ready);
}
