#pragma once

#include <lua.h>
#include <lualib.h>

#include <atomic>
#include <chrono>
#include <cstddef>
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
        bool runScript(std::string_view src, std::string_view chunkName, int deadlineMs = 250);
        bool protectedCall(int nargs, int nresults, std::string_view context, int deadlineMs = 50);
        void runOnMain(std::function<void()> fn);
        void assertMainThread() const;

        // Shutdown hooks run (last-in, first-out) during destruction, before lua_close.
        void registerShutdownHook(std::function<void()> fn);

        std::string const& getOrCompileBytecode(std::string const& key, std::string const& source);

        std::size_t memoryUsage() const { return m_memoryUsage; }
        std::size_t memoryLimit() const { return m_memoryLimit; }
        void setMemoryLimit(std::size_t bytes) { m_memoryLimit = bytes; }

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

        lua_State* m_state = nullptr;
        std::thread::id m_ownerThread;
        std::atomic<bool> m_ready{false};

        std::chrono::steady_clock::time_point m_scriptDeadline{};
        int m_scriptBudgetMs = 0;

        std::size_t m_memoryUsage = 0;
        std::size_t m_memoryLimit = 64 * 1024 * 1024;

        int m_tracebackRef = 0;

        bool m_codegenEnabled = false;

        std::unordered_map<std::string, std::string> m_bytecodeCache;

        std::unique_ptr<Requirer> m_requirer;

        std::vector<std::function<void()>> m_shutdownHooks;
    };
}
