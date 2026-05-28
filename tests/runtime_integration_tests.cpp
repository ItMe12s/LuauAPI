#include <LuauAPI.hpp>

#include "lua/Runtime.hpp"
#include "lua/bindings/internal/Fields.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
    class HostRuntimeScope {
    public:
        HostRuntimeScope() {
            luax::Runtime::resetForHostTests();
        }

        ~HostRuntimeScope() {
            luax::Runtime::resetForHostTests();
        }

        HostRuntimeScope(HostRuntimeScope const&) = delete;
        HostRuntimeScope& operator=(HostRuntimeScope const&) = delete;
    };

    std::filesystem::path makeTempDir(char const* name) {
        auto dir = std::filesystem::temp_directory_path()
            / (std::string("luauapi_") + name + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
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

TEST_CASE("runtime shutdown blocks recreate") {
    HostRuntimeScope scope;

    REQUIRE(luax::Runtime::getOrCreate() != nullptr);
    REQUIRE(luax::Runtime::isInitialized());

    luax::Runtime::shutdown();

    REQUIRE(luax::Runtime::isShuttingDown());
    REQUIRE(luax::Runtime::getOrCreate() == nullptr);
    REQUIRE(luax::Runtime::getIfInitialized() == nullptr);
}

TEST_CASE("public api returns shutdown error after shutdown") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("shutdown_api");

    luax::Runtime::shutdown();

    auto result = imes::luauapi::runScript(dir, "return 1", "Bootstrap.luau", 0);

    REQUIRE(result.isErr());
    REQUIRE(contains(result.unwrapErr(), "shutting down"));
    REQUIRE(luax::Runtime::getIfInitialized() == nullptr);

    std::filesystem::remove_all(dir);
}

TEST_CASE("runScript reports success and script failures") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("run_script");

    auto ok = imes::luauapi::runScript(dir, "_G.luauapi_value = 7", "Bootstrap.luau", 0);
    REQUIRE(ok.isOk());

    auto syntax = imes::luauapi::runScript(dir, "function", "Syntax.luau", 0);
    REQUIRE(syntax.isErr());

    auto runtime = imes::luauapi::runScript(dir, "error('boom')", "Runtime.luau", 0);
    REQUIRE(runtime.isErr());
    REQUIRE(contains(runtime.unwrapErr(), "boom"));

    std::filesystem::remove_all(dir);
}

TEST_CASE("protectedCall validates stack shape") {
    HostRuntimeScope scope;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    REQUIRE_FALSE(runtime->protectedCall(0, 0, "empty stack", 0));
    REQUIRE(contains(runtime->lastError(), "protectedCall missing function"));
}

TEST_CASE("require post-resume errors restore script budget") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("require_budget");
    writeFile(dir / "Module.luau", "return 1, 2");

    auto result = imes::luauapi::runScript(dir, "return require('Module')", "Bootstrap.luau", luax::kDefaultScriptDeadlineMs);

    REQUIRE(result.isErr());
    REQUIRE(contains(result.unwrapErr(), "must return a single value"));
    auto* runtime = luax::Runtime::getIfInitialized();
    REQUIRE(runtime != nullptr);
    REQUIRE(runtime->scriptBudgetDepthForHostTests() == 0);

    std::filesystem::remove_all(dir);
}

TEST_CASE("runFile uses changed source after cache entry exists") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("cache_invalidation");
    auto file = dir / "Bootstrap.luau";

    writeFile(file, "_G.luauapi_cache_value = 1");
    REQUIRE(imes::luauapi::runFile(dir, "Bootstrap.luau", 0).isOk());

    writeFile(file, "_G.luauapi_cache_value = 2");
    REQUIRE(imes::luauapi::runFile(dir, "Bootstrap.luau", 0).isOk());

    auto* runtime = luax::Runtime::getIfInitialized();
    REQUIRE(runtime != nullptr);
    lua_State* L = runtime->state();
    REQUIRE(L != nullptr);
    lua_getglobal(L, "luauapi_cache_value");
    REQUIRE(lua_tointeger(L, -1) == 2);
    lua_pop(L, 1);
    REQUIRE(runtime->bytecodeCacheSizeForHostTests() >= 2);

    std::filesystem::remove_all(dir);
}

TEST_CASE("async api after shutdown does not recreate runtime before polling") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("async_shutdown");

    luax::Runtime::shutdown();

    auto future = imes::luauapi::runScriptAsync(dir, "return 1", "Bootstrap.luau", 0);
    (void)future;

    REQUIRE(luax::Runtime::getIfInitialized() == nullptr);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ccnode fields persist and evict by node pointer") {
    HostRuntimeScope scope;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    auto* node = reinterpret_cast<cocos2d::CCNode*>(static_cast<std::uintptr_t>(0x1234));
    luax::Fields::push(L, node);
    REQUIRE(lua_istable(L, -1));
    lua_pushinteger(L, 9);
    lua_setfield(L, -2, "calls");
    lua_pop(L, 1);

    luax::Fields::push(L, node);
    lua_getfield(L, -1, "calls");
    REQUIRE(lua_tointeger(L, -1) == 9);
    lua_pop(L, 2);

    luax::Fields::evict(node);
    luax::Fields::push(L, node);
    lua_getfield(L, -1, "calls");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 2);
}

TEST_CASE("ccnode fields evict on final release") {
    HostRuntimeScope scope;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    cocos2d::CCNode node;
    node.m_uReference = 1;
    luax::Fields::push(L, &node);
    REQUIRE(lua_istable(L, -1));
    lua_pushinteger(L, 3);
    lua_setfield(L, -2, "calls");
    lua_pop(L, 1);

    node.release();

    luax::Fields::push(L, &node);
    lua_getfield(L, -1, "calls");
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 2);
}
