#pragma once

#include "lua/Config.hpp"
#include "lua/bindings/framework/BindingHost.hpp"

#include <Geode/Geode.hpp>
#include <RuntimeTypes.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <list>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace luax {
    class Requirer;

    class Runtime final : public BindingHost {
    public:
        Runtime();
        ~Runtime();

        Runtime(Runtime const&) = delete;
        Runtime& operator=(Runtime const&) = delete;

        static Runtime* getOrCreate();
        static bool isInitialized();
        static Runtime* getIfInitialized();
        static void shutdown();
        static bool isShuttingDown();
#if defined(LUAUAPI_HOST_TESTS)
        static void resetForTests();
        void setStatusForTests(imes::luauapi::RuntimeStatus status);
#endif
        static void setMainThreadId(std::thread::id id);
        static bool isMainThread();

        lua_State* state() override;
        bool ready() const override;
        imes::luauapi::RuntimeStatus status() const;
        bool assertMainThread() const;

        geode::Result<void> runScript(
            std::string_view src, std::string_view chunkName, int deadlineMs = kDefaultScriptDeadlineMs
        );
        geode::Result<void> protectedCall(
            int nargs, int nresults, std::string_view context, int deadlineMs = kDefaultScriptDeadlineMs
        ) override;
        geode::Result<void> protectedCallWithTraceback(
            int nargs, int nresults, std::string_view context
        ) override;
        static std::string compileSource(std::string_view source);

        // Only outermost guard sets script budget/deadline.
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

        using ResourcesRootScope = BindingHost::ResourcesRootScope;

        void setResourcesRoot(std::filesystem::path const& root) override;
        void swapResourcesRoot(std::filesystem::path& root) override;

        std::filesystem::path const& resourcesRoot() const override {
            return m_resourcesRoot;
        }

        void clearLastError() {
            m_lastError.clear();
        }

        std::string lastError() const {
            return m_lastError;
        }

        void registerShutdownHook(std::function<void()> fn) override;

        geode::Result<std::reference_wrapper<std::string const>> getOrCompileBytecode(
            std::string const& key, std::string_view source
        );

        std::size_t bytecodeCacheBytes() const {
            return m_bytecodeCacheBytes;
        }

        std::size_t memoryUsage() const {
            return m_memoryUsage;
        }

        std::size_t memoryLimit() const {
            return m_memoryLimit;
        }

        bool codegenEnabled() const {
            return m_codegenEnabled;
        }

        std::uint32_t generation() const {
            return m_generation;
        }

    private:
        static void* boundedAlloc(void* ud, void* ptr, size_t osize, size_t nsize);
        static void interruptCallback(lua_State* L, int gc);
        static void panicCallback(lua_State* L, int errcode);
        static int luaTraceback(lua_State* L);
        static int luaPrint(lua_State* L);
        static int luaLoadstring(lua_State* L);

        void installTraceback();
        void installPrint();
        void installLoadstring();

        struct BytecodeCacheEntry {
            std::string key;
            std::string bytecode;
        };

        std::string formatLuaError(char const* chunk);
        void setLastError(std::string error);
        geode::Result<void> failWith(std::string error);
        geode::Result<void> cachedError() const;
        void runShutdownHooks();
        void removeBytecodeCacheEntry(std::list<BytecodeCacheEntry>::iterator it);
        void trimBytecodeCacheForInsert(std::size_t incomingBytes);
        bool reserveExternalMemory(std::size_t bytes);
        void releaseExternalMemory(std::size_t bytes);
        bool tryCacheCompiledBytecode(std::string const& key, std::string compiled, long long compileMs);

        lua_State* m_state = nullptr;
        std::thread::id m_ownerThread;
        std::atomic<imes::luauapi::RuntimeStatus> m_status{imes::luauapi::RuntimeStatus::NotReady};
        bool m_destroyed = false;
        std::uint32_t m_generation = 0;

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

        std::list<BytecodeCacheEntry> m_bytecodeLru;
        std::unordered_map<std::string, std::list<BytecodeCacheEntry>::iterator> m_bytecodeIndex;
        std::size_t m_bytecodeCacheBytes = 0;
        std::string m_bytecodeScratch;

#if !defined(LUAUAPI_HOST_TESTS)
        std::unique_ptr<Requirer> m_requirer;
#endif

        std::vector<std::function<void()>> m_shutdownHooks;
    };
} // namespace luax
