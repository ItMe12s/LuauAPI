#include "bindings/geode/CurrentMod.hpp"
#include "bindings/geode/ModSandbox.hpp"
#include "core/Runtime.hpp"
#include "framework/stack/Stack.hpp"
#include "require/PathSandbox.hpp"

#include <Geode/loader/Mod.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <string_view>
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
            ("luauapi_mod_sandbox_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }

    struct ResolveArgs {
        std::string root;
        std::string path;
        bool requireWritable = false;
    };

    int resolveSandboxTargetEntry(lua_State* L) {
        auto const* args = static_cast<ResolveArgs const*>(lua_touserdata(L, lua_upvalueindex(1)));
        lua_pushlstring(L, args->root.data(), args->root.size());
        lua_pushlstring(L, args->path.data(), args->path.size());
        auto target = luax::resolveSandboxTarget(L, 1, 2, "test", args->requireWritable);
        if (!target) {
            return 2;
        }
        luax::push(L, luax::filesystemPathString(target->path));
        lua_pushboolean(L, target->writable);
        return 2;
    }

    bool callResolve(lua_State* L, ResolveArgs const& args, int& outResults) {
        lua_settop(L, 0);
        lua_pushlightuserdata(L, const_cast<ResolveArgs*>(&args));
        lua_pushcclosure(L, resolveSandboxTargetEntry, "resolveSandboxTargetEntry", 1);
        if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
            outResults = 1;
            return false;
        }
        outResults = lua_gettop(L);
        return true;
    }

    void ensureWritableRoots(geode::Mod* mod) {
        REQUIRE(std::filesystem::create_directories(mod->getSaveDir()));
        REQUIRE(std::filesystem::create_directories(mod->getConfigDir()));
        REQUIRE(std::filesystem::create_directories(mod->getPersistentDir()));
    }

    std::filesystem::path expectedResolvedPath(
        std::filesystem::path const& root, std::string_view relative
    ) {
        auto resolved = luax::resolveInsideRoot(root, relative);
        REQUIRE(resolved.isOk());
        return resolved.unwrap();
    }
} // namespace

TEST_CASE("resolveSandboxTarget maps writable mod roots") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    ensureWritableRoots(mod);

    for (auto const* root : {"save", "config", "persistent"}) {
        ResolveArgs args{root, "nested/file.txt", true};
        int results = 0;
        REQUIRE(callResolve(L, args, results));
        REQUIRE(results == 2);
        REQUIRE(lua_isstring(L, 1));
        REQUIRE(lua_toboolean(L, 2));

        auto expected = expectedResolvedPath(mod->getResourcesDir() / root, "nested/file.txt");
        REQUIRE(std::string(lua_tostring(L, 1)) == luax::filesystemPathString(expected));
        lua_pop(L, results);
    }

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("resolveSandboxTarget maps read-only resources root") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();

    ResolveArgs args{"resources", "asset.txt", false};
    int results = 0;
    REQUIRE(callResolve(L, args, results));
    REQUIRE(results == 2);
    REQUIRE(lua_isstring(L, 1));
    REQUIRE_FALSE(lua_toboolean(L, 2));

    auto expected = expectedResolvedPath(mod->getResourcesDir(), "asset.txt");
    REQUIRE(std::string(lua_tostring(L, 1)) == luax::filesystemPathString(expected));
    lua_pop(L, results);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("resolveSandboxTarget rejects writable requests against resources") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();

    ResolveArgs args{"resources", "blocked.txt", true};
    int results = 0;
    REQUIRE(callResolve(L, args, results));
    REQUIRE(results == 2);
    REQUIRE(lua_isnil(L, 1));
    REQUIRE(lua_isstring(L, 2));
    REQUIRE(std::string(lua_tostring(L, 2)) == "root is read-only");
    lua_pop(L, results);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("resolveSandboxTarget rejects path escapes") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    REQUIRE(std::filesystem::create_directories(mod->getSaveDir()));

    ResolveArgs args{"save", "../escape.txt", false};
    int results = 0;
    REQUIRE(callResolve(L, args, results));
    REQUIRE(results == 2);
    REQUIRE(lua_isnil(L, 1));
    REQUIRE(lua_isstring(L, 2));
    REQUIRE(std::string(lua_tostring(L, 2)).find("escapes") != std::string::npos);
    lua_pop(L, results);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("resolveSandboxTarget errors when no current mod matches runtime resources root") {
    RuntimeGuard guard;
    auto dir = makeTempDir();

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();

    ResolveArgs args{"save", "file.txt", false};
    int results = 0;
    REQUIRE_FALSE(callResolve(L, args, results));
    REQUIRE(results == 1);
    REQUIRE(lua_isstring(L, 1));
    REQUIRE(std::string(lua_tostring(L, 1)) == "current mod is unavailable");
    lua_pop(L, results);

    std::filesystem::remove_all(dir);
}

TEST_CASE("resolveSandboxTarget rejects absolute relative paths") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    REQUIRE(std::filesystem::create_directories(mod->getSaveDir()));

    auto absolutePath = dir.root_path() / "absolute" / "path.txt";
    ResolveArgs args{"save", luax::filesystemPathString(absolutePath), false};
    int results = 0;
    REQUIRE(callResolve(L, args, results));
    REQUIRE(results == 2);
    REQUIRE(lua_isnil(L, 1));
    REQUIRE(lua_isstring(L, 2));
    REQUIRE(std::string(lua_tostring(L, 2)) == "path must be relative");
    lua_pop(L, results);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("resolveSandboxTarget rejects empty relative paths") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    REQUIRE(std::filesystem::create_directories(mod->getSaveDir()));

    ResolveArgs args{"save", "", false};
    int results = 0;
    REQUIRE(callResolve(L, args, results));
    REQUIRE(results == 2);
    REQUIRE(lua_isnil(L, 1));
    REQUIRE(lua_isstring(L, 2));
    REQUIRE(std::string(lua_tostring(L, 2)) == "path is empty");
    lua_pop(L, results);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}

TEST_CASE("resolveSandboxTarget errors on unknown root") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();

    ResolveArgs args{"unknown", "file.txt", false};
    int results = 0;
    REQUIRE_FALSE(callResolve(L, args, results));
    REQUIRE(results == 1);
    REQUIRE(lua_isstring(L, 1));
    auto err = std::string(lua_tostring(L, 1));
    REQUIRE(err.find("unknown root") != std::string::npos);
    REQUIRE(err.find("unknown") != std::string::npos);
    lua_pop(L, results);

    std::filesystem::remove_all(dir);
    geode::Mod::destroy(mod);
}
