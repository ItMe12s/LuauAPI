#include "framework/Binding.hpp"
#include "framework/usertype/LuaRef.hpp"
#include "framework/schedule/ScheduledHandleBinding.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "bindings/task/TaskScheduler.hpp"
#include "core/Runtime.hpp"

#include <Geode/Geode.hpp>
#include <chrono>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;

    struct TaskHandleTraits {
        using Scheduler = TaskScheduler;
        static constexpr char const* kMeta = "luax.TaskHandle";
        static constexpr char const* kTypeName = "TaskHandle";

        static constexpr int userdataTag() noexcept {
            return detail::taskHandleTag();
        }
    };

    using TaskHandleBinding = ScheduledHandleBinding<TaskHandleTraits>;

    std::chrono::steady_clock::time_point& timeOrigin() {
        static std::chrono::steady_clock::time_point origin = std::chrono::steady_clock::now();
        return origin;
    }

    void pushHandle(lua_State* L, std::uint64_t id) {
        TaskHandleBinding::push(L, id);
    }

    void ensureCapacity(lua_State* L) {
        if (TaskScheduler::get().full()) {
            luaL_error(
                L, "task: too many scheduled tasks (limit %d)", static_cast<int>(kMaxScheduledTasks)
            );
        }
    }

    bool s_armWarned = false;

    void ensureTaskTickArmed() {
        if (armTaskTick()) {
            s_armWarned = false;
            return;
        }
        if (!s_armWarned) {
            s_armWarned = true;
            geode::log::warn(
                "task: scheduler tick not armed yet (game scene not ready); callbacks will run "
                "once the scene is available"
            );
        }
    }

    int taskSpawn(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        int nargs = lua_gettop(L) - 1;
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || !runtime->ready()) {
            luaL_error(L, "task.spawn requires an initialized runtime");
        }
        (void)runtime->protectedCall(nargs, 0, "task.spawn", kHookScriptDeadlineMs);
        return 0;
    }

    int taskDelay(lua_State* L) {
        double seconds = luaL_checknumber(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);
        if (seconds < 0.0) seconds = 0.0;
        ensureCapacity(L);
        ensureTaskTickArmed();
        LuaRef ref;
        ref.reset(L, 2);
        std::uint64_t id = TaskScheduler::get().add(std::move(ref), seconds, 0.0);
        pushHandle(L, id);
        return 1;
    }

    int taskEvery(lua_State* L) {
        double seconds = luaL_checknumber(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);
        if (seconds <= 0.0) {
            luaL_error(L, "task.every: interval must be > 0");
        }
        ensureCapacity(L);
        ensureTaskTickArmed();
        LuaRef ref;
        ref.reset(L, 2);
        std::uint64_t id = TaskScheduler::get().add(std::move(ref), seconds, seconds);
        pushHandle(L, id);
        return 1;
    }

    int taskDefer(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        ensureCapacity(L);
        ensureTaskTickArmed();
        LuaRef ref;
        ref.reset(L, 1);
        std::uint64_t id = TaskScheduler::get().addDeferred(std::move(ref));
        pushHandle(L, id);
        return 1;
    }

    int taskCancel(lua_State* L) {
        return TaskHandleBinding::luaCancel(L);
    }

    int timeNow(lua_State* L) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - timeOrigin();
        lua_pushnumber(L, elapsed.count());
        return 1;
    }

    int timeUnix(lua_State* L) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        std::chrono::duration<double> seconds = now;
        lua_pushnumber(L, seconds.count());
        return 1;
    }

    void registerHandleMetatable(lua_State* L) {
        TaskHandleBinding::registerMetatable(L);
    }
} // namespace

namespace luax {
    geode::Result<void> registerTask(lua_State* L) {
        timeOrigin() = std::chrono::steady_clock::now();

        registerHandleMetatable(L);

        lua_newtable(L);
        setTableCFunction(L, -1, "spawn", &taskSpawn);
        setTableCFunction(L, -1, "delay", &taskDelay);
        setTableCFunction(L, -1, "every", &taskEvery);
        setTableCFunction(L, -1, "defer", &taskDefer);
        setTableCFunction(L, -1, "cancel", &taskCancel);
        lua_setglobal(L, "task");

        lua_newtable(L);
        setTableCFunction(L, -1, "now", &timeNow);
        setTableCFunction(L, -1, "unix", &timeUnix);
        lua_setglobal(L, "time");

        ensureTaskTickArmed();
        if (auto* runtime = static_cast<Runtime*>(lua_callbacks(L)->userdata)) {
            runtime->registerShutdownHook([] {
                disarmTaskTick();
                TaskScheduler::get().clear();
            });
        }

        return geode::Ok();
    }
} // namespace luax

LUAX_BINDING(task_lib, registerTask)
