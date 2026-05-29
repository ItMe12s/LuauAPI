#include <LuauAPI.hpp>

#include "lua/Runtime.hpp"
#include "lua/bindings/task/TaskScheduler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace {
    class HostRuntimeScope {
    public:
        HostRuntimeScope() {
            luax::Runtime::resetForHostTests();
            luax::TaskScheduler::get().clear();
        }

        ~HostRuntimeScope() {
            luax::TaskScheduler::get().clear();
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

    bool contains(std::string const& text, std::string const& needle) {
        return text.find(needle) != std::string::npos;
    }

    int globalInt(lua_State* L, char const* name) {
        lua_getglobal(L, name);
        int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    lua_State* runAndGetState(std::filesystem::path const& dir, std::string const& src) {
        auto ok = imes::luauapi::runScript(dir, src, "TaskTest.luau", 0);
        REQUIRE(ok.isOk());
        auto* runtime = luax::Runtime::getIfInitialized();
        REQUIRE(runtime != nullptr);
        auto* L = runtime->state();
        REQUIRE(L != nullptr);
        return L;
    }
}

TEST_CASE("task.delay fires once after its delay then is removed") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("task_delay");

    auto* L = runAndGetState(dir, "_G.fires = 0\n task.delay(1.0, function() _G.fires += 1 end)");
    auto& sched = luax::TaskScheduler::get();
    REQUIRE(sched.activeCount() == 1);

    sched.advance(0.5, L);
    REQUIRE(globalInt(L, "fires") == 0);

    sched.advance(0.6, L);
    REQUIRE(globalInt(L, "fires") == 1);
    REQUIRE(sched.activeCount() == 0);

    sched.advance(2.0, L);
    REQUIRE(globalInt(L, "fires") == 1);

    std::filesystem::remove_all(dir);
}

TEST_CASE("task.every repeats every interval") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("task_every");

    auto* L = runAndGetState(dir, "_G.fires = 0\n task.every(0.5, function() _G.fires += 1 end)");
    auto& sched = luax::TaskScheduler::get();

    sched.advance(0.5, L);
    sched.advance(0.5, L);
    sched.advance(0.5, L);
    REQUIRE(globalInt(L, "fires") == 3);
    REQUIRE(sched.activeCount() == 1);

    std::filesystem::remove_all(dir);
}

TEST_CASE("task.defer fires on next tick regardless of dt") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("task_defer");

    auto* L = runAndGetState(dir, "_G.fires = 0\n task.defer(function() _G.fires += 1 end)");
    auto& sched = luax::TaskScheduler::get();

    sched.advance(0.0, L);
    REQUIRE(globalInt(L, "fires") == 1);
    REQUIRE(sched.activeCount() == 0);

    std::filesystem::remove_all(dir);
}

TEST_CASE("task.cancel stops a pending task") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("task_cancel");

    auto* L = runAndGetState(dir,
        "_G.fires = 0\n local h = task.delay(1.0, function() _G.fires += 1 end)\n h:cancel()");
    auto& sched = luax::TaskScheduler::get();
    REQUIRE(sched.activeCount() == 0);

    sched.advance(2.0, L);
    REQUIRE(globalInt(L, "fires") == 0);

    std::filesystem::remove_all(dir);
}

TEST_CASE("erroring callback is dropped and others still run") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("task_error");

    auto* L = runAndGetState(dir,
        "_G.good = 0\n task.delay(0.1, function() error('boom') end)\n task.delay(0.1, function() _G.good += 1 end)");
    auto& sched = luax::TaskScheduler::get();
    REQUIRE(sched.activeCount() == 2);

    sched.advance(0.2, L);
    REQUIRE(globalInt(L, "good") == 1);
    REQUIRE(sched.activeCount() == 0);

    std::filesystem::remove_all(dir);
}

TEST_CASE("scheduling beyond the cap raises a lua error") {
    HostRuntimeScope scope;
    auto dir = makeTempDir("task_cap");

    auto* L = runAndGetState(dir,
        "_G.cap_err = nil\n local ok, err = pcall(function()\n"
        "  for i = 1, 5000 do task.delay(100, function() end) end\n"
        "end)\n _G.ok = ok\n _G.cap_err = tostring(err)");

    lua_getglobal(L, "ok");
    REQUIRE(lua_toboolean(L, -1) == 0);
    lua_pop(L, 1);

    lua_getglobal(L, "cap_err");
    std::string err = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
    lua_pop(L, 1);
    REQUIRE(contains(err, "too many scheduled tasks"));

    REQUIRE(luax::TaskScheduler::get().activeCount() == luax::kMaxScheduledTasks);

    std::filesystem::remove_all(dir);
}
