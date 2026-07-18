#include "bindings/geode/CurrentMod.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "host/lua_test_helpers.hpp"

#include <Geode/loader/Mod.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <string>

namespace luax {
    geode::Result<void> registerGeodeFs(lua_State* L);
    geode::Result<void> registerGeodeJson(lua_State* L);
} // namespace luax

namespace {
    using RuntimeGuard = luauapi_test::BindingModRuntimeGuard;

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

    int callJsonParse(lua_State* L, std::string const& text) {
        lua_getglobal(L, "geode");
        lua_getfield(L, -1, "json");
        lua_getfield(L, -1, "parse");
        lua_pushlstring(L, text.data(), text.size());
        return lua_pcall(L, 1, 2, 0);
    }

    int callFs(
        lua_State* L, char const* method, std::string const& root, std::string const& rel,
        std::optional<std::string> data = std::nullopt
    ) {
        lua_settop(L, 0);
        lua_getglobal(L, "geode");
        lua_getfield(L, -1, "fs");
        lua_getfield(L, -1, method);
        lua_remove(L, 1);
        lua_remove(L, 1);
        lua_pushlstring(L, root.data(), root.size());
        lua_pushlstring(L, rel.data(), rel.size());
        int nargs = 2;
        if (data) {
            lua_pushlstring(L, data->data(), data->size());
            nargs = 3;
        }
        return lua_pcall(L, nargs, LUA_MULTRET, 0);
    }

    std::string readFileContents(std::filesystem::path const& path) {
        std::ifstream in(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    void writeFileContents(std::filesystem::path const& path, std::string const& data) {
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.good());
        out << data;
    }
} // namespace

TEST_CASE("Runtime bytecode cache tracks total bytes and evicts by size") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    std::string payload(32 * 1024, 'x');
    std::string source = "return '" + payload + "'";

    for (int i = 0; i < 3; ++i) {
        auto key = "key_" + std::to_string(i);
        REQUIRE(runtime->getOrCompileBytecode(key, source).isOk());
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
    REQUIRE(runtime->getOrCompileBytecode("memory_usage_key", source).isOk());
    REQUIRE(runtime->memoryUsage() > before);
    REQUIRE(runtime->memoryUsage() >= runtime->bytecodeCacheBytes());
}

TEST_CASE("Runtime bytecode cache hits bypass compile budget checks") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    std::string source = "return 99";
    REQUIRE(runtime->getOrCompileBytecode("cache_hit_key", source).isOk());
    REQUIRE(runtime->getOrCompileBytecode("cache_hit_key", source).isOk());
}

TEST_CASE("geode.fs.read rejects directories") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto nested = dir.path / "save" / "nested";
    REQUIRE(std::filesystem::create_directories(nested));

    REQUIRE(callFsRead(L, "nested") == 0);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));
    std::string err(lua_tostring(L, -1));
    REQUIRE(err.find("regular file") != std::string::npos);
    lua_pop(L, 2);

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.list returns nil err for open and type failures") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto saveDir = dir.path / "save";
    REQUIRE(std::filesystem::create_directories(saveDir));

    REQUIRE(callFs(L, "list", "save", "missing") == 0);
    REQUIRE(lua_gettop(L) == 2);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));

    writeFileContents(saveDir / "file.txt", "x");
    REQUIRE(callFs(L, "list", "save", "file.txt") == 0);
    REQUIRE(lua_gettop(L) == 2);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.list rejects directories with too many name bytes") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto listDir = dir.path / "save" / "long_names";
    REQUIRE(std::filesystem::create_directories(listDir));

    for (std::size_t i = 0; i < luax::kMaxFsListEntries; ++i) {
        std::string name(60, 'a');
        name += std::to_string(i);
        name += ".txt";
        std::ofstream out(listDir / name);
        REQUIRE(out.good());
        out << "x";
    }

    REQUIRE(callFs(L, "list", "save", "long_names") == 0);
    REQUIRE(lua_gettop(L) == 2);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));
    std::string err(lua_tostring(L, -1));
    REQUIRE(err.find("name bytes") != std::string::npos);
    lua_pop(L, 2);

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.list rejects directories with too many entries") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto listDir = dir.path / "save" / "many";
    REQUIRE(std::filesystem::create_directories(listDir));
    for (std::size_t i = 0; i <= luax::kMaxFsListEntries; ++i) {
        std::ofstream out(listDir / ("entry_" + std::to_string(i) + ".txt"));
        out << "x";
    }

    REQUIRE(callFs(L, "list", "save", "many") == 0);
    REQUIRE(lua_gettop(L) == 2);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));
    std::string err(lua_tostring(L, -1));
    REQUIRE(err.find("maximum entries") != std::string::npos);
    lua_pop(L, 2);

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

TEST_CASE("geode.fs.read rejects file size above kMaxFsReadBytes") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto saveDir = dir.path / "save";
    REQUIRE(std::filesystem::create_directories(saveDir));
    writeFileContents(saveDir / "oversized.bin", std::string(luax::kMaxFsReadBytes + 1, 'x'));

    REQUIRE(callFsRead(L, "oversized.bin") == 0);
    REQUIRE(lua_isnil(L, -2));
    REQUIRE(lua_isstring(L, -1));
    std::string err(lua_tostring(L, -1));
    REQUIRE(err.find("maximum read size") != std::string::npos);
    lua_pop(L, 2);

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.read cap applies to save and config roots") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto oversized = std::string(luax::kMaxFsReadBytes + 1, 'y');
    for (auto const* root : {"save", "config"}) {
        auto rootDir = dir.path / root;
        REQUIRE(std::filesystem::create_directories(rootDir));
        writeFileContents(rootDir / "oversized.bin", oversized);

        lua_settop(L, 0);
        lua_getglobal(L, "geode");
        lua_getfield(L, -1, "fs");
        lua_getfield(L, -1, "read");
        lua_remove(L, 1);
        lua_remove(L, 1);
        lua_pushlstring(L, root, std::char_traits<char>::length(root));
        lua_pushlstring(L, "oversized.bin", 13);
        REQUIRE(lua_pcall(L, 2, 2, 0) == 0);
        REQUIRE(lua_isnil(L, -2));
        REQUIRE(lua_isstring(L, -1));
        std::string err(lua_tostring(L, -1));
        REQUIRE(err.find("maximum read size") != std::string::npos);
        lua_pop(L, 2);
    }

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.write rejects payload size above kMaxFsWriteBytes") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    REQUIRE(std::filesystem::create_directories(dir.path / "save"));

    auto oversized = std::string(luax::kMaxFsWriteBytes + 1, 'z');
    REQUIRE(callFs(L, "write", "save", "oversized.bin", oversized) == 0);
    REQUIRE(lua_gettop(L) == 2);
    REQUIRE(lua_isnil(L, 1));
    REQUIRE(lua_isstring(L, 2));
    std::string err(lua_tostring(L, 2));
    REQUIRE(err.find("maximum write size") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(dir.path / "save" / "oversized.bin"));

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.write stores a file inside the writable save root") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto saveDir = dir.path / "save";
    REQUIRE(std::filesystem::create_directories(saveDir));

    REQUIRE(callFs(L, "write", "save", "note.txt", "hello") == 0);
    REQUIRE(lua_gettop(L) == 1);
    REQUIRE(lua_toboolean(L, 1));
    REQUIRE(readFileContents(saveDir / "note.txt") == "hello");

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.write rejects paths that escape the root") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    REQUIRE(std::filesystem::create_directories(dir.path / "save"));

    REQUIRE(callFs(L, "write", "save", "../escape.txt", "nope") == 0);
    REQUIRE(lua_gettop(L) == 2);
    REQUIRE(lua_isnil(L, 1));
    REQUIRE(lua_isstring(L, 2));
    std::string err(lua_tostring(L, 2));
    REQUIRE(err.find("escapes") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(dir.path / "escape.txt"));

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.write rejects the read-only resources root") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    REQUIRE(callFs(L, "write", "resources", "blocked.txt", "nope") == 0);
    REQUIRE(lua_gettop(L) == 2);
    REQUIRE(lua_isnil(L, 1));
    REQUIRE(lua_isstring(L, 2));
    std::string err(lua_tostring(L, 2));
    REQUIRE(err.find("read-only") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(dir.path / "blocked.txt"));

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.exists reports whether a sandboxed path is present") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto saveDir = dir.path / "save";
    REQUIRE(std::filesystem::create_directories(saveDir));
    writeFileContents(saveDir / "present.txt", "x");

    REQUIRE(callFs(L, "exists", "save", "present.txt") == 0);
    REQUIRE(lua_gettop(L) == 1);
    REQUIRE(lua_toboolean(L, 1));

    REQUIRE(callFs(L, "exists", "save", "absent.txt") == 0);
    REQUIRE(lua_gettop(L) == 1);
    REQUIRE_FALSE(lua_toboolean(L, 1));

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.mkdir creates a directory inside the writable root") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    REQUIRE(std::filesystem::create_directories(dir.path / "save"));

    REQUIRE(callFs(L, "mkdir", "save", "made") == 0);
    REQUIRE(lua_gettop(L) == 1);
    REQUIRE(lua_toboolean(L, 1));
    REQUIRE(std::filesystem::is_directory(dir.path / "save" / "made"));

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.fs.remove deletes an entry inside the writable root") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_resource_bounds_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto saveDir = dir.path / "save";
    REQUIRE(std::filesystem::create_directories(saveDir));
    auto victim = saveDir / "gone.txt";
    writeFileContents(victim, "x");
    REQUIRE(std::filesystem::exists(victim));

    REQUIRE(callFs(L, "remove", "save", "gone.txt") == 0);
    REQUIRE(lua_gettop(L) == 1);
    REQUIRE(lua_toboolean(L, 1));
    REQUIRE_FALSE(std::filesystem::exists(victim));

    geode::Mod::destroy(mod);
}

TEST_CASE("geode.json.dump serializes a value through matjson") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerResourceBindings(L);

    auto dumpValue = [&](auto pushArg) {
        lua_settop(L, 0);
        lua_getglobal(L, "geode");
        lua_getfield(L, -1, "json");
        lua_getfield(L, -1, "dump");
        lua_remove(L, 1);
        lua_remove(L, 1);
        pushArg();
        REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
        REQUIRE(lua_isstring(L, -1));
        return std::string(lua_tostring(L, -1));
    };

    REQUIRE(dumpValue([&] {
                lua_pushboolean(L, 1);
            }) == "true");
    REQUIRE(dumpValue([&] {
                lua_pushboolean(L, 0);
            }) == "false");
    REQUIRE(dumpValue([&] {
                lua_newtable(L);
            }) == "{}");
}
