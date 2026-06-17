#include "core/Runtime.hpp"
#include "host/lua_test_helpers.hpp"

#include <LuauAPI.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <thread>

namespace {
    using RuntimeGuard = luauapi_test::RuntimeGuard;
} // namespace

TEST_CASE("runScript executes source from resources root") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_api_tests_"};
    auto root = temp.path / "run_script";
    std::filesystem::create_directories(root);

    auto result = imes::luauapi::runScript(root, "return 17", "script.luau");
    REQUIRE(result.isOk());
}

TEST_CASE("runScript rejects oversized source") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_api_tests_"};
    auto root = temp.path / "run_script_oversized";
    std::filesystem::create_directories(root);

    std::string source(luax::kMaxScriptBytes + 1, 'a');
    auto result = imes::luauapi::runScript(root, source, "big.luau");
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr().find("maximum size") != std::string::npos);
}

TEST_CASE("runFile loads and executes a flat script resource") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_api_tests_"};
    auto root = temp.path / "run_file";
    std::filesystem::create_directories(root);
    luauapi_test::writeTestFile(root / "entry.luau", "return 42");

    auto result = imes::luauapi::runFile(root, "entry.luau");
    REQUIRE(result.isOk());
}

TEST_CASE("runFile rejects missing script") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_api_tests_"};
    auto root = temp.path / "run_file_missing";
    std::filesystem::create_directories(root);

    auto result = imes::luauapi::runFile(root, "missing.luau");
    REQUIRE(result.isErr());
}

TEST_CASE("api entry points require the main thread") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_api_tests_"};
    auto root = temp.path / "main_thread";
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

TEST_CASE("api rejects work while runtime is shutting down") {
    RuntimeGuard guard;
    luax::Runtime::getOrCreate();
    luauapi_test::ScopedTempDir temp{"luauapi_api_tests_"};
    auto root = temp.path / "shutdown";
    std::filesystem::create_directories(root);

    luax::Runtime::shutdown();
    REQUIRE(luax::Runtime::isShuttingDown());

    auto scriptResult = imes::luauapi::runScript(root, "return 1", "shutdown.luau");
    REQUIRE(scriptResult.isErr());
    REQUIRE(scriptResult.unwrapErr() == "luau runtime shutting down");

    luauapi_test::writeTestFile(root / "entry.luau", "return 2");
    auto fileResult = imes::luauapi::runFile(root, "entry.luau");
    REQUIRE(fileResult.isErr());
    REQUIRE(fileResult.unwrapErr() == "luau runtime shutting down");

    REQUIRE_FALSE(imes::luauapi::isReady());
    REQUIRE(imes::luauapi::status() == imes::luauapi::RuntimeStatus::NotReady);
}
