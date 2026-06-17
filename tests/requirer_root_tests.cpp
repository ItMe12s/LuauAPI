#include "core/Runtime.hpp"
#include "host/lua_test_helpers.hpp"
#include "require/Requirer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <lua.h>
#include <string>

namespace {
    using RuntimeGuard = luauapi_test::RuntimeGuard;

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

    struct LoadModuleCtx {
        luax::Requirer* req;
        std::string chunkname;
        std::string loadname;
    };

    int callLoadModule(lua_State* L) {
        auto* ctx = static_cast<LoadModuleCtx*>(lua_tolightuserdata(L, lua_upvalueindex(1)));
        return ctx->req->loadModule(L, ctx->chunkname.c_str(), ctx->loadname.c_str());
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
    REQUIRE(emptyRootErr.unwrapErr() == "resources root is not configured");
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

TEST_CASE("Requirer loadModule honors primed pending contents") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    lua_State* L = runtime->state();
    REQUIRE(L != nullptr);

    auto dir = makeTempDir();
    writeFile(dir / "Mod.luau", "return 42");

    luax::Requirer req(*runtime);
    req.setResourcesRoot(dir);
    REQUIRE(req.toChild("Mod") == NAVIGATE_SUCCESS);

    char cacheKeyBuffer[1024];
    size_t cacheKeySize = 0;
    REQUIRE(req.writeCacheKey(cacheKeyBuffer, sizeof(cacheKeyBuffer), &cacheKeySize) == WRITE_SUCCESS);

    auto chunk = req.chunkname();
    LoadModuleCtx ctx{&req, chunk, chunk};

    int base = lua_gettop(L);
    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, callLoadModule, "callLoadModule", 1);
    int status = lua_pcall(L, 0, LUA_MULTRET, 0);

    REQUIRE(status == 0);
    REQUIRE(lua_gettop(L) - base == 1);
    REQUIRE(lua_isnumber(L, -1));
    REQUIRE(lua_tonumber(L, -1) == 42.0);
    lua_settop(L, base);

    std::filesystem::remove_all(dir);
}
