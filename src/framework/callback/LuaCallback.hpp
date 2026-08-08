#pragma once

#include "core/Runtime.hpp"
#include "framework/usertype/LuaRef.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace luax {
    inline void logCallbackFailure(std::string_view context) {
        geode::log::warn("[lua:{}] callback failed", context);
    }

    class LuaCallback {
    public:
        using PushArgsFn = void (*)(lua_State* L, void* ctx);
        using PopResultsFn = void (*)(lua_State* L, void* ctx);

        LuaCallback() = default;

        LuaCallback(lua_State* L, int index) {
            reset(L, index);
        }

        void reset(lua_State* L, int index) {
            m_ref = std::make_shared<LuaRef>(L, index);
        }

        void reset() {
            m_ref.reset();
        }

        bool valid() const {
            return m_ref && m_ref->valid();
        }

        static bool fire(LuaRef& callback, std::string_view context, int deadlineMs) {
            auto* runtime = Runtime::getIfInitialized();
            if (!runtime) return false;
            auto* L = callback.luaState();
            if (!L) return false;
            int top = lua_gettop(L);
            if (!callback.push()) return false;
            Runtime::ResourcesRootScope scope(*runtime, callback.resourcesRoot());
            bool ok = runtime->protectedCall(L, 0, 0, context, deadlineMs).isOk();
            lua_settop(L, top);
            return ok;
        }

        // L top must be fn + nargs args. Restores L to the stack below those values.
        static bool fireStackOnThread(
            lua_State* L, int nargs, std::string_view context, int deadlineMs,
            std::filesystem::path const& resourcesRoot
        ) {
            if (!L) return false;
            int const base = lua_gettop(L) - nargs - 1;
            if (base < 0 || !lua_isfunction(L, base + 1)) return false;

            if (Runtime::isShuttingDown()) {
                lua_settop(L, base);
                return false;
            }
            auto* runtime = Runtime::getIfInitialized();
            if (!runtime || !runtime->ready()) {
                lua_settop(L, base);
                return false;
            }

            lua_State* co = lua_newthread(L);
            lua_insert(L, base + 1);
            lua_xmove(L, co, nargs + 1);

            bool const ok = resumeAndReport(*runtime, co, nargs, context, deadlineMs, resourcesRoot);
            lua_settop(L, base);
            return ok;
        }

        static bool fireOnThread(LuaRef& callback, std::string_view context, int deadlineMs) {
            auto* L = callback.luaState();
            if (!L || !callback.push()) return false;
            return fireStackOnThread(L, 0, context, deadlineMs, callback.resourcesRoot());
        }

        static bool resumeThread(
            LuaRef& threadRef, double resumeValue, std::string_view context, int deadlineMs
        ) {
            if (Runtime::isShuttingDown()) return false;
            auto* runtime = Runtime::getIfInitialized();
            if (!runtime || !runtime->ready()) return false;
            auto* L = threadRef.luaState();
            if (!L || !threadRef.push()) return false;

            bool ok = false;
            if (auto* co = lua_tothread(L, -1)) {
                lua_pushnumber(co, resumeValue);
                ok = resumeAndReport(*runtime, co, 1, context, deadlineMs, threadRef.resourcesRoot());
            }
            lua_pop(L, 1);
            return ok;
        }

        bool invoke(
            int nargs, int nresults, std::string_view context, int deadlineMs,
            PushArgsFn pushArgs = nullptr, void* pushCtx = nullptr,
            PopResultsFn popResults = nullptr, void* popCtx = nullptr
        ) const {
            if (Runtime::isShuttingDown()) return false;
            auto* runtime = Runtime::getIfInitialized();
            if (!runtime || !runtime->ready()) return false;
            auto* L = runtime->state();
            if (!L || !m_ref) return false;

            int top = lua_gettop(L);
            if (!m_ref->push()) return false;
            if (pushArgs) {
                pushArgs(L, pushCtx);
            }
            Runtime::ResourcesRootScope scope(*runtime, m_ref->resourcesRoot());
            auto result = runtime->protectedCall(L, nargs, nresults, context, deadlineMs);
            if (result.isOk() && popResults && nresults > 0) {
                popResults(L, popCtx);
            }
            lua_settop(L, top);
            return result.isOk();
        }

        std::shared_ptr<LuaRef> const& ref() const {
            return m_ref;
        }

    private:
        static bool resumeAndReport(
            Runtime& runtime, lua_State* co, int nargs, std::string_view context, int deadlineMs,
            std::filesystem::path const& resourcesRoot
        ) {
            Runtime::ResourcesRootScope scope(runtime, resourcesRoot);
            int status;
            {
                Runtime::ScriptBudgetGuard budget(runtime, deadlineMs);
                status = lua_resume(co, nullptr, nargs);
            }
            return reportThreadStatus(runtime, co, status, context);
        }

        static bool reportThreadStatus(
            Runtime& runtime, lua_State* co, int status, std::string_view context
        ) {
            if (status == LUA_OK || status == LUA_YIELD) return true;
            char const* err = lua_tostring(co, -1);
            char const* trace = lua_debugtrace(co);
            std::string message = err ? err : "(unknown error)";
            if (trace && *trace) {
                message.append("\n");
                message.append(trace);
            }
            runtime.reportError(context, message);
            return false;
        }

        std::shared_ptr<LuaRef> m_ref;
    };
} // namespace luax
