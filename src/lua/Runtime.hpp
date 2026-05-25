#pragma once

#include "Config.hpp"

#include <imes.luauapi/LuauAPI.hpp>
#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace luax {
    class Requirer;

    class Runtime final {
    public:
        Runtime();
        ~Runtime();

        Runtime(Runtime const&) = delete;
        Runtime& operator=(Runtime const&) = delete;

        static Runtime& instance();

        lua_State* state();
        bool ready() const;
        imes::luauapi::RuntimeStatus status() const;
        static bool isInitialized();
        static Runtime* getIfInitialized();
        static void setMainThreadId(std::thread::id id);
        static bool isMainThread();
        bool runScript(std::string_view src, std::string_view chunkName, int deadlineMs = kDefaultScriptDeadlineMs);
        geode::Result<void> runBytecode(std::string const& bytecode, std::string_view chunkName, int deadlineMs = kDefaultScriptDeadlineMs);
        static std::string compileSource(std::string_view source);
        bool protectedCall(int nargs, int nresults, std::string_view context, int deadlineMs = 50);
        void runOnMain(std::function<void()> fn);
        bool assertMainThread() const;

        class ScriptBudgetGuard final {
        public:
            ScriptBudgetGuard(Runtime& runtime, int deadlineMs);
            ~ScriptBudgetGuard();

            ScriptBudgetGuard(ScriptBudgetGuard const&) = delete;
            ScriptBudgetGuard& operator=(ScriptBudgetGuard const&) = delete;

        private:
            Runtime& m_runtime;
            bool m_outermost = false;
            int m_previousBudget = 0;
            std::chrono::steady_clock::time_point m_previousDeadline{};
        };

        class ResourcesRootScope final {
        public:
            ResourcesRootScope(Runtime& runtime, std::filesystem::path root);
            ~ResourcesRootScope();

            ResourcesRootScope(ResourcesRootScope const&) = delete;
            ResourcesRootScope& operator=(ResourcesRootScope const&) = delete;

        private:
            Runtime& m_runtime;
            std::filesystem::path m_previous;
        };

        void setResourcesRoot(std::filesystem::path const& root);
        std::filesystem::path const& resourcesRoot() const { return m_resourcesRoot; }
        void clearLastError() { m_lastError.clear(); }
        std::string const& lastError() const { return m_lastError; }

        // Shutdown hooks run (last-in, first-out) during destruction, before lua_close.
        void registerShutdownHook(std::function<void()> fn);

        std::string const& getOrCompileBytecode(std::string const& key, std::string const& source);

        std::size_t memoryUsage() const { return m_memoryUsage; }
        std::size_t memoryLimit() const { return m_memoryLimit; }

        bool codegenEnabled() const { return m_codegenEnabled; }

    private:
        static void* boundedAlloc(void* ud, void* ptr, size_t osize, size_t nsize);
        static void interruptCallback(lua_State* L, int gc);
        static void panicCallback(lua_State* L, int errcode);
        static int luaTraceback(lua_State* L);
        static int luaPrint(lua_State* L);

        void installTraceback();
        void installPrint();
        std::string formatLuaError(char const* chunk);
        void setLastError(std::string error);

        lua_State* m_state = nullptr;
        std::thread::id m_ownerThread;
        std::atomic<imes::luauapi::RuntimeStatus> m_status{imes::luauapi::RuntimeStatus::NotReady};

        std::chrono::steady_clock::time_point m_scriptDeadline{};
        int m_scriptBudgetMs = 0;
        int m_scriptBudgetDepth = 0;

        std::size_t m_memoryUsage = 0;
        std::size_t m_memoryLimit = kMemoryLimitBytes;

        int m_tracebackRef = 0;

        bool m_codegenEnabled = false;
        std::filesystem::path m_resourcesRoot;
        std::string m_lastError;
        std::string m_initError;

        std::unordered_map<std::string, std::string> m_bytecodeCache;

        std::unique_ptr<Requirer> m_requirer;

        std::vector<std::function<void()>> m_shutdownHooks;
    };
}
