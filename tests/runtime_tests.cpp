#include "core/Runtime.hpp"
#include "host/lua_test_helpers.hpp"
#include "require/PathSandbox.hpp"

#include <Geode/log.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {
    using RuntimeGuard = luauapi_test::RuntimeGuard;

    void pushReturnValue(lua_State* L, int value) {
        luauapi_test::loadFunction(L, std::string("return ") + std::to_string(value));
    }

    void requireNoRootLeak(std::string const& text, std::filesystem::path const& root) {
        auto rootResult = luax::canonicalRoot(root);
        REQUIRE(rootResult.isOk());
        auto rootText = luax::filesystemPathString(rootResult.unwrap());
        auto genericRoot = luax::normalizedPathString(rootResult.unwrap());

        REQUIRE(text.find(rootText) == std::string::npos);
        if (genericRoot != rootText) {
            REQUIRE(text.find(genericRoot) == std::string::npos);
        }
    }
} // namespace

TEST_CASE("Runtime initializes and shuts down cleanly") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();

    REQUIRE(runtime != nullptr);
    REQUIRE(luax::Runtime::isInitialized());
    REQUIRE(runtime->ready());
    REQUIRE(runtime->status() == imes::luauapi::RuntimeStatus::Ready);
    REQUIRE(runtime->state() != nullptr);
}

TEST_CASE("Runtime shutdown clears initialization state") {
    {
        RuntimeGuard guard;
        REQUIRE(luax::Runtime::getOrCreate() != nullptr);
        REQUIRE(luax::Runtime::isInitialized());
    }

    REQUIRE_FALSE(luax::Runtime::isInitialized());
    REQUIRE(luax::Runtime::getIfInitialized() == nullptr);
}

TEST_CASE("Runtime rejects execution while shutting down") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    luax::Runtime::setShuttingDownForTests(true);

    auto scriptResult = runtime->runScript("return 1", "@shutdown.luau");
    REQUIRE(scriptResult.isErr());
    REQUIRE(runtime->lastError() == "luau runtime shutting down");

    pushReturnValue(L, 7);
    auto callResult = runtime->protectedCall(L, 0, 1, "shutdown");
    REQUIRE(callResult.isErr());
    REQUIRE(runtime->lastError() == "luau runtime shutting down");

    pushReturnValue(L, 8);
    auto tracebackResult = runtime->protectedCallWithTraceback(L, 0, 1, "shutdown");
    REQUIRE(tracebackResult.isErr());
    REQUIRE(runtime->lastError() == "luau runtime shutting down");
}

TEST_CASE("Runtime shutdown clears initialization state and blocks recreation") {
    RuntimeGuard guard;
    REQUIRE(luax::Runtime::getOrCreate() != nullptr);

    luax::Runtime::shutdown();
    REQUIRE(luax::Runtime::isShuttingDown());
    REQUIRE(luax::Runtime::getOrCreate() == nullptr);
    REQUIRE(luax::Runtime::getIfInitialized() == nullptr);
}

TEST_CASE("protectedCall restores stack height on success") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    pushReturnValue(L, 99);
    int topBefore = lua_gettop(L);
    REQUIRE(runtime->protectedCall(L, 0, 1, "test").isOk());
    REQUIRE(lua_gettop(L) == topBefore);
    REQUIRE(lua_tointeger(L, -1) == 99);
    lua_pop(L, 1);
    REQUIRE(lua_gettop(L) == topBefore - 1);
}

TEST_CASE("protectedCall restores stack height on failure") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    lua_pushcfunction(
        L,
        [](lua_State* state) -> int {
            luaL_error(state, "boom");
            return 0;
        },
        "fail"
    );
    int topBefore = lua_gettop(L);
    REQUIRE(runtime->protectedCall(L, 0, 0, "test").isErr());
    REQUIRE(lua_gettop(L) == topBefore - 1);
    REQUIRE_FALSE(runtime->lastError().empty());
}

TEST_CASE("runScript executes and reports errors") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();

    REQUIRE(runtime->runScript("return 5", "@ok.luau").isOk());
    REQUIRE(runtime->lastError().empty());

    REQUIRE(runtime->runScript("return (", "@bad.luau").isErr());
    REQUIRE_FALSE(runtime->lastError().empty());
}

TEST_CASE("runtime errors redact resource-root paths") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_runtime_"};
    auto nested = dir.path / "nested";
    REQUIRE(std::filesystem::create_directories(nested));
    auto scriptPath = nested / "secret.luau";
    {
        std::ofstream file(scriptPath);
        file << "error path fixture";
    }

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    geode::log::clearCapturedMessages();

    auto pathText = luax::filesystemPathString(scriptPath);
    auto source = std::string("local function explode()\n") + "    error([=[" + pathText +
        "]=])\n" + "end\n" + "explode()";
    auto result = runtime->runScript(source, "@" + pathText);

    REQUIRE(result.isErr());
    auto err = runtime->lastError();
    requireNoRootLeak(err, dir.path);
    REQUIRE(err.find("secret.luau") != std::string::npos);

    auto const& errors = geode::log::capturedErrorMessages();
    REQUIRE_FALSE(errors.empty());
    requireNoRootLeak(errors.back(), dir.path);
    REQUIRE(errors.back().find("secret.luau") != std::string::npos);
}

TEST_CASE("print output redacts resource-root paths") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_runtime_"};
    auto scriptPath = dir.path / "print_secret.luau";
    {
        std::ofstream file(scriptPath);
        file << "print path fixture";
    }

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    geode::log::clearCapturedMessages();

    auto pathText = luax::filesystemPathString(scriptPath);
    auto source = std::string("print([=[") + pathText + "]=])";
    auto before = geode::log::capturedInfoMessages().size();
    REQUIRE(runtime->runScript(source, "@print.luau").isOk());

    auto const& messages = geode::log::capturedInfoMessages();
    REQUIRE(messages.size() == before + 1);
    auto const& line = messages.back();
    requireNoRootLeak(line, dir.path);
    REQUIRE(line.find("print_secret.luau") != std::string::npos);
}

TEST_CASE("warn output redacts resource-root paths") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_runtime_"};
    auto scriptPath = dir.path / "warn_secret.luau";
    {
        std::ofstream file(scriptPath);
        file << "warn path fixture";
    }

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    geode::log::clearCapturedMessages();

    auto pathText = luax::filesystemPathString(scriptPath);
    auto source = std::string("warn([=[") + pathText + "]=])";
    auto before = geode::log::capturedWarnMessages().size();
    REQUIRE(runtime->runScript(source, "@warn.luau").isOk());

    auto const& messages = geode::log::capturedWarnMessages();
    REQUIRE(messages.size() == before + 1);
    auto const& line = messages.back();
    requireNoRootLeak(line, dir.path);
    REQUIRE(line.find("warn_secret.luau") != std::string::npos);
}

TEST_CASE("Runtime rejects off-main-thread access") {
    RuntimeGuard guard;
    luax::Runtime::getOrCreate();

    std::optional<bool> offThreadAccess;
    std::thread worker([&] {
        auto* runtime = luax::Runtime::getIfInitialized();
        offThreadAccess = runtime && runtime->assertMainThread();
    });
    worker.join();

    REQUIRE(offThreadAccess.has_value());
    REQUIRE_FALSE(*offThreadAccess);
}

TEST_CASE("protectedCallWithTraceback rejects panicked runtime without executing callable") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    static int executed = 0;
    auto pushTrackedCallable = [&] {
        executed = 0;
        lua_pushcfunction(
            L,
            [](lua_State* state) -> int {
                ++executed;
                lua_pushinteger(state, 42);
                return 1;
            },
            "shouldNotRun"
        );
    };

    SECTION("default panic error") {
        pushTrackedCallable();
        int topBefore = lua_gettop(L);

        runtime->clearLastError();
        runtime->setStatusForTests(imes::luauapi::RuntimeStatus::Panicked);

        REQUIRE(runtime->protectedCallWithTraceback(L, 0, 1, "test").isErr());
        REQUIRE(executed == 0);
        REQUIRE(lua_gettop(L) == topBefore);
        REQUIRE(runtime->lastError().find("luau runtime panicked") != std::string::npos);

        lua_pop(L, 1);
    }

    SECTION("cached panic error") {
        lua_pushcfunction(
            L,
            [](lua_State* state) -> int {
                luaL_error(state, "setup boom");
                return 0;
            },
            "setupFail"
        );
        REQUIRE(runtime->protectedCall(L, 0, 0, "setup").isErr());
        auto cached = runtime->lastError();
        REQUIRE_FALSE(cached.empty());

        runtime->setStatusForTests(imes::luauapi::RuntimeStatus::Panicked);
        pushTrackedCallable();

        REQUIRE(runtime->protectedCallWithTraceback(L, 0, 1, "test").isErr());
        REQUIRE(executed == 0);
        REQUIRE(runtime->lastError() == cached);

        lua_pop(L, 1);
    }
}

TEST_CASE("protectedCall works from sandbox thread") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* GL = runtime->state();
    REQUIRE(GL != nullptr);

    lua_State* SL = lua_newthread(GL);
    luaL_sandboxthread(SL);

    luauapi_test::loadFunction(SL, "return function() return 42 end");
    REQUIRE(lua_pcall(SL, 0, 1, 0) == 0);
    REQUIRE(lua_isfunction(SL, -1));

    int topBefore = lua_gettop(SL);
    REQUIRE(runtime->protectedCall(SL, 0, 1, "sandbox-test").isOk());
    REQUIRE(lua_gettop(SL) == topBefore);
    REQUIRE(lua_tointeger(SL, -1) == 42);
    lua_pop(SL, 1);
    lua_pop(GL, 1);
}

TEST_CASE("shutdown hooks run in LIFO order") {
    std::vector<int> order;
    {
        RuntimeGuard guard;
        auto* runtime = luax::Runtime::getOrCreate();
        runtime->registerShutdownHook([&] {
            order.push_back(1);
        });
        runtime->registerShutdownHook([&] {
            order.push_back(2);
        });
        runtime->registerShutdownHook([&] {
            order.push_back(3);
        });
        runtime->runShutdownHooksForTests();
    }

    REQUIRE(order == std::vector<int>{3, 2, 1});
}

TEST_CASE("shutdown hooks registered during shutdown defer to next runShutdownHooks") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    bool outerRan = false;
    bool nestedRan = false;

    runtime->registerShutdownHook([&] {
        outerRan = true;
        runtime->registerShutdownHook([&] {
            nestedRan = true;
        });
    });

    runtime->runShutdownHooksForTests();
    REQUIRE(outerRan);
    REQUIRE_FALSE(nestedRan);

    runtime->runShutdownHooksForTests();
    REQUIRE(nestedRan);
}

TEST_CASE("protectedCall from sandbox thread restores stack on failure") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* GL = runtime->state();
    REQUIRE(GL != nullptr);

    lua_State* SL = lua_newthread(GL);
    luaL_sandboxthread(SL);

    luauapi_test::loadFunction(SL, "return function() error('boom') end");
    REQUIRE(lua_pcall(SL, 0, 1, 0) == 0);
    REQUIRE(lua_isfunction(SL, -1));

    int topBefore = lua_gettop(SL);
    REQUIRE(runtime->protectedCall(SL, 0, 0, "sandbox-fail").isErr());
    REQUIRE(lua_gettop(SL) == topBefore - 1);
    REQUIRE_FALSE(runtime->lastError().empty());
    lua_pop(GL, 1);
}
