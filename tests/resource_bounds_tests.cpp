#include "lua/bindings/Binding.hpp"
#include "lua/bindings/geode/CurrentMod.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/loader/Mod.hpp>

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace luax {
    geode::Result<void> registerGeodeFs(lua_State* L);
    geode::Result<void> registerGeodeJson(lua_State* L);
}

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::resetBindingsForTests();
        }

        ~RuntimeGuard() {
            luax::invalidateCurrentModCache();
            geode::Mod::resetForTests();
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };

    std::filesystem::path makeTempDir() {
        auto dir = std::filesystem::temp_directory_path()
            / ("luauapi_resource_bounds_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }

    void registerResourceBindings(lua_State* L) {
        luax::registerBinding({"geode_fs", &luax::registerGeodeFs, 0});
        luax::registerBinding({"geode_json", &luax::registerGeodeJson, 0});
        REQUIRE(luax::applyAllBindings(L) == std::nullopt);
    }

    int callFsRead(lua_State* L, std::string const& rel) {
        lua_getglobal(L, "geode");
        lua_getfield(L, -1, "fs");
        lua_getfield(L, -1, "read");
        lua_pushlstring(L, "save", 4);
        lua_pushlstring(L, rel.data(), rel.size());
        return lua_pcall(L, 2, 2, 0);
    }

    int callFsList(lua_State* L, std::string const& rel) {
        lua_getglobal(L, "geode");
        lua_getfield(L, -1, "fs");
        lua_getfield(L, -1, "list");
        lua_pushlstring(L, "save", 4);
        lua_pushlstring(L, rel.data(), rel.size());
        return lua_pcall(L, 2, 2, 0);
    }

    int callJsonParse(lua_State* L, std::string const& text) {
        lua_getglobal(L, "geode");
        lua_getfield(L, -1, "json");
        lua_getfield(L, -1, "parse");
        lua_pushlstring(L, text.data(), text.size());
        return lua_pcall(L, 1, 2, 0);
    }
}

TEST_CASE("Runtime bytecode cache tracks total bytes and evicts by size") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    std::string payload(32 * 1024, 'x');
    std::string source = "return '" + payload + "'";

    for (int i = 0; i < 3; ++i) {
        auto key = "key_" + std::to_string(i);
        bool ok = false;
        (void)runtime->getOrCompileBytecode(key, source, ok);
        REQUIRE(ok);
    }

    REQUIRE(runtime->bytecodeCacheBytes() <= luax::kMaxBytecodeCacheBytes);
    REQUIRE(runtime->bytecodeCacheBytes() > 0);
    REQUIRE(runtime->memoryUsage() >= runtime->bytecodeCacheBytes());
}

TEST_CASE("Runtime bytecode cache counts toward memory usage") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    std::size_t before = runtime->memoryUsage();
    std::string source = "return 42";
    bool ok = false;
    (void)runtime->getOrCompileBytecode("memory_usage_key", source, ok);
    REQUIRE(ok);
    REQUIRE(runtime->memoryUsage() > before);
    REQUIRE(runtime->memoryUsage() >= runtime->bytecodeCacheBytes());
}

TEST_CASE("Runtime bytecode cache hits bypass compile budget checks") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    std::string source = "return 99";
    bool ok = false;
    (void)runtime->getOrCompileBytecode("cache_hit_key", source, ok);
    REQUIRE(ok);
    (void)runtime->getOrCompileBytecode("cache_hit_key", source, ok);
    REQUIRE(ok);
}

TEST_CASE("geode.fs.read rejects directories") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto nested = dir / "save" / "nested";
    REQUIRE(std::filesystem::create_directories(nested));

    REQUIRE(callFsRead(L, "nested") == 0);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));
    std::string err(lua_tostring(L, -1));
    REQUIRE(err.find("regular file") != std::string::npos);
    lua_pop(L, 2);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.list rejects directories with too many name bytes") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto listDir = dir / "save" / "long_names";
    REQUIRE(std::filesystem::create_directories(listDir));

    for (std::size_t i = 0; i < luax::kMaxFsListEntries; ++i) {
        std::string name(60, 'a');
        name += std::to_string(i);
        name += ".txt";
        std::ofstream out(listDir / name);
        REQUIRE(out.good());
        out << "x";
    }

    REQUIRE(callFsList(L, "long_names") == 0);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));
    std::string err(lua_tostring(L, -1));
    REQUIRE(err.find("name bytes") != std::string::npos);
    lua_pop(L, 2);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.list rejects directories with too many entries") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto listDir = dir / "save" / "many";
    REQUIRE(std::filesystem::create_directories(listDir));
    for (std::size_t i = 0; i <= luax::kMaxFsListEntries; ++i) {
        std::ofstream out(listDir / ("entry_" + std::to_string(i) + ".txt"));
        out << "x";
    }

    REQUIRE(callFsList(L, "many") == 0);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));
    std::string err(lua_tostring(L, -1));
    REQUIRE(err.find("maximum entries") != std::string::npos);
    lua_pop(L, 2);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("geode.json.parse rejects oversized input") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerResourceBindings(L);

    std::string oversized(luax::kMaxJsonParseBytes + 1, '{');
    REQUIRE(callJsonParse(L, oversized) == 0);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));
    std::string err(lua_tostring(L, -1));
    REQUIRE(err.find("maximum size") != std::string::npos);
    lua_pop(L, 2);
}
