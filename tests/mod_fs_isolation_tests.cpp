#include "bindings/geode/CurrentMod.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"

#include <Geode/loader/Mod.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <string>
#include <thread>

namespace luax {
    geode::Result<void> registerGeodeFs(lua_State* L);
} // namespace luax

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

    std::filesystem::path makeTempDir(std::string_view prefix) {
        auto dir = std::filesystem::temp_directory_path() /
            (std::string(prefix) +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }

    void registerFsBindings(lua_State* L) {
        luax::registerBinding({"geode_fs", &luax::registerGeodeFs, 0});
        REQUIRE(luax::applyAllBindings(L) == std::nullopt);
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

    struct ModPair {
        std::filesystem::path parent;
        geode::Mod* modA = nullptr;
        geode::Mod* modB = nullptr;

        ModPair() {
            parent = makeTempDir("luauapi_mod_fs_isolation_");
            modA = geode::Mod::create(parent / "modA");
            modB = geode::Mod::create(parent / "modB");
            REQUIRE(std::filesystem::create_directories(modA->getSaveDir()));
            REQUIRE(std::filesystem::create_directories(modB->getSaveDir()));
        }

        ~ModPair() {
            std::filesystem::remove_all(parent);
            if (modA) geode::Mod::destroy(modA);
            if (modB) geode::Mod::destroy(modB);
        }
    };
} // namespace

TEST_CASE("geode.fs.read returns per-mod content for the same relative save path") {
    RuntimeGuard guard;
    ModPair mods;

    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerFsBindings(L);

    runtime->setResourcesRoot(mods.modA->getResourcesDir());
    REQUIRE(callFs(L, "write", "save", "note.txt", "mod-a") == 0);
    REQUIRE(lua_toboolean(L, 1));

    runtime->setResourcesRoot(mods.modB->getResourcesDir());
    REQUIRE(callFs(L, "write", "save", "note.txt", "mod-b") == 0);
    REQUIRE(lua_toboolean(L, 1));

    runtime->setResourcesRoot(mods.modA->getResourcesDir());
    REQUIRE(callFs(L, "read", "save", "note.txt") == 0);
    REQUIRE(lua_isstring(L, 1));
    REQUIRE(std::string(lua_tostring(L, 1)) == "mod-a");

    runtime->setResourcesRoot(mods.modB->getResourcesDir());
    REQUIRE(callFs(L, "read", "save", "note.txt") == 0);
    REQUIRE(lua_isstring(L, 1));
    REQUIRE(std::string(lua_tostring(L, 1)) == "mod-b");
}

TEST_CASE("geode.fs.write under one mod does not touch another mod save root") {
    RuntimeGuard guard;
    ModPair mods;

    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerFsBindings(L);

    runtime->setResourcesRoot(mods.modA->getResourcesDir());
    REQUIRE(callFs(L, "write", "save", "note.txt", "from-a") == 0);
    REQUIRE(readFileContents(mods.modA->getSaveDir() / "note.txt") == "from-a");
    REQUIRE_FALSE(std::filesystem::exists(mods.modB->getSaveDir() / "note.txt"));

    runtime->setResourcesRoot(mods.modB->getResourcesDir());
    REQUIRE(callFs(L, "write", "save", "note.txt", "from-b") == 0);
    REQUIRE(readFileContents(mods.modB->getSaveDir() / "note.txt") == "from-b");
    REQUIRE(readFileContents(mods.modA->getSaveDir() / "note.txt") == "from-a");
}

TEST_CASE("geode.fs rejects traversal from one mod toward another mod") {
    RuntimeGuard guard;
    ModPair mods;

    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerFsBindings(L);

    {
        std::ofstream out(mods.modB->getSaveDir() / "secret.txt");
        out << "hidden";
        REQUIRE(out.good());
    }

    auto const traversal =
        std::filesystem::relative(mods.modB->getSaveDir() / "secret.txt", mods.modA->getSaveDir());
    runtime->setResourcesRoot(mods.modA->getResourcesDir());
    REQUIRE(callFs(L, "read", "save", traversal.generic_string()) == 0);
    REQUIRE(lua_isnil(L, 1));
    REQUIRE(lua_isstring(L, 2));
    std::string err(lua_tostring(L, 2));
    REQUIRE(err.find("escapes") != std::string::npos);
    REQUIRE(readFileContents(mods.modB->getSaveDir() / "secret.txt") == "hidden");
}

TEST_CASE("geode.fs.exists under one mod does not see files in another mod") {
    RuntimeGuard guard;
    ModPair mods;

    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerFsBindings(L);

    std::ofstream out(mods.modB->getSaveDir() / "only-b.txt");
    out << "x";
    REQUIRE(out.good());

    runtime->setResourcesRoot(mods.modA->getResourcesDir());
    REQUIRE(callFs(L, "exists", "save", "only-b.txt") == 0);
    REQUIRE(lua_gettop(L) == 1);
    REQUIRE_FALSE(lua_toboolean(L, 1));
}

TEST_CASE("switching resources root changes which mod geode.fs targets") {
    RuntimeGuard guard;
    ModPair mods;

    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerFsBindings(L);

    runtime->setResourcesRoot(mods.modA->getResourcesDir());
    REQUIRE(callFs(L, "write", "save", "note.txt", "before-switch") == 0);
    REQUIRE(readFileContents(mods.modA->getSaveDir() / "note.txt") == "before-switch");

    runtime->setResourcesRoot(mods.modB->getResourcesDir());
    REQUIRE(callFs(L, "write", "save", "note.txt", "after-switch") == 0);
    REQUIRE(readFileContents(mods.modB->getSaveDir() / "note.txt") == "after-switch");
    REQUIRE(readFileContents(mods.modA->getSaveDir() / "note.txt") == "before-switch");

    runtime->setResourcesRoot(mods.modA->getResourcesDir());
    REQUIRE(callFs(L, "read", "save", "note.txt") == 0);
    REQUIRE(lua_isstring(L, 1));
    REQUIRE(std::string(lua_tostring(L, 1)) == "before-switch");
}
