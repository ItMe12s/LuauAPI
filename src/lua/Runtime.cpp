#include "Runtime.hpp"

#include "Binding.hpp"
#include "Requirer.hpp"

#include <Geode/Geode.hpp>
#include <Luau/CodeGen.h>
#include <Luau/Compiler.h>
#include <Luau/Require.h>
#include <lua.h>
#include <lualib.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <utility>

namespace luax {
    namespace {
        constexpr char const kTracebackName[] = "luax:traceback";

        class StackGuard {
        public:
            explicit StackGuard(lua_State* L) : m_state(L), m_top(lua_gettop(L)) {}
            ~StackGuard() {
                if (m_active) {
                    lua_settop(m_state, m_top);
                }
            }

            StackGuard(StackGuard const&) = delete;
            StackGuard& operator=(StackGuard const&) = delete;

            void dismiss() { m_active = false; }

        private:
            lua_State* m_state = nullptr;
            int m_top = 0;
            bool m_active = true;
        };

        class ScriptBudgetGuard {
        public:
            ScriptBudgetGuard(
                int& budget,
                std::chrono::steady_clock::time_point& deadline,
                int deadlineMs
            ) : m_budget(budget),
                m_deadline(deadline),
                m_previousBudget(budget),
                m_previousDeadline(deadline) {
                m_budget = deadlineMs;
                if (deadlineMs > 0) {
                    m_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(deadlineMs);
                }
            }

            ~ScriptBudgetGuard() {
                m_budget = m_previousBudget;
                m_deadline = m_previousDeadline;
            }

            ScriptBudgetGuard(ScriptBudgetGuard const&) = delete;
            ScriptBudgetGuard& operator=(ScriptBudgetGuard const&) = delete;

        private:
            int& m_budget;
            std::chrono::steady_clock::time_point& m_deadline;
            int m_previousBudget = 0;
            std::chrono::steady_clock::time_point m_previousDeadline{};
        };

        std::string valueToString(lua_State* L, int idx) {
            size_t len = 0;
            char const* text = luaL_tolstring(L, idx, &len);
            std::string out = text ? std::string(text, len) : std::string();
            lua_pop(L, 1);
            return out;
        }
    }

    Runtime::Runtime() : m_ownerThread(std::this_thread::get_id()) {
        m_state = lua_newstate(&Runtime::boundedAlloc, this);
        if (!m_state) {
            geode::log::error("luau lua_newstate failed");
            std::abort();
        }

        luaL_openlibs(m_state);

        if (Luau::CodeGen::isSupported()) {
            Luau::CodeGen::create(m_state);
            m_codegenEnabled = true;
            geode::log::info("luau codegen enabled");
        } else {
            geode::log::info("luau codegen not supported on this platform, using interpreter");
        }

        installTraceback();
        installPrint();

        auto* cb = lua_callbacks(m_state);
        cb->interrupt = &Runtime::interruptCallback;
        cb->panic = &Runtime::panicCallback;
        cb->userdata = this;

        m_requirer = std::make_unique<Requirer>(*this);
        luaopen_require(m_state, &Requirer::initConfig, m_requirer.get());

        applyAllBindings(m_state);

        // Mark globals as safe for fastcall, must be last.
        lua_pushvalue(m_state, LUA_GLOBALSINDEX);
        lua_setsafeenv(m_state, -1, true);
        lua_pop(m_state, 1);

        m_ready.store(true, std::memory_order_release);
        geode::log::info("luau runtime ready");
    }

    Runtime::~Runtime() {
        for (auto it = m_shutdownHooks.rbegin(); it != m_shutdownHooks.rend(); ++it) {
            (*it)();
        }
        m_shutdownHooks.clear();

        if (m_state) {
            lua_close(m_state);
            m_state = nullptr;
        }
        geode::log::info("luau runtime shutdown");
    }

    void Runtime::registerShutdownHook(std::function<void()> fn) {
        assertMainThread();
        if (fn) m_shutdownHooks.push_back(std::move(fn));
    }

    Runtime& Runtime::instance() {
        static std::optional<Runtime> runtime;
        if (!runtime) {
            runtime.emplace();
        }
        return *runtime;
    }

    lua_State* Runtime::state() {
        assertMainThread();
        return m_state;
    }

    bool Runtime::ready() const {
        return m_ready.load(std::memory_order_acquire);
    }

    void Runtime::installTraceback() {
        lua_pushcfunction(m_state, &Runtime::luaTraceback, kTracebackName);
        m_tracebackRef = lua_ref(m_state, -1);
        lua_pop(m_state, 1);
    }

    void Runtime::installPrint() {
        lua_pushcfunction(m_state, &Runtime::luaPrint, "print");
        lua_setglobal(m_state, "print");
    }

    int Runtime::luaPrint(lua_State* L) {
        StackGuard stack(L);
        int argc = lua_gettop(L);
        std::string out;

        for (int i = 1; i <= argc; ++i) {
            if (i > 1) out.push_back('\t');
            out.append(valueToString(L, i));
        }

        geode::log::info("[lua] {}", out);
        return 0;
    }

    int Runtime::luaTraceback(lua_State* L) {
        char const* msg = lua_tostring(L, 1);
        std::string out;
        if (msg) {
            out.assign(msg);
        } else {
            out.assign("(non-string error)");
        }

        lua_Debug ar;
        out.append("\n  stack:");
        for (int level = 0; lua_getinfo(L, level, "sln", &ar); ++level) {
            out.append("\n    ");
            if (ar.source) {
                out.append(ar.short_src);
            }
            if (ar.currentline > 0) {
                out.append(":");
                out.append(std::to_string(ar.currentline));
            }
            if (ar.name) {
                out.append(" in ");
                out.append(ar.name);
            }
        }

        lua_pushlstring(L, out.data(), out.size());
        return 1;
    }

    std::string Runtime::formatLuaError(char const* chunk) {
        char const* raw = lua_tostring(m_state, -1);
        std::string out;
        if (chunk) {
            out.append("[");
            out.append(chunk);
            out.append("] ");
        }
        out.append(raw ? raw : "(unknown error)");
        return out;
    }

    std::string const& Runtime::getOrCompileBytecode(std::string const& key, std::string const& source) {
        auto cached = m_bytecodeCache.find(key);
        if (cached != m_bytecodeCache.end()) {
            return cached->second;
        }

        auto compileStart = std::chrono::steady_clock::now();
        Luau::CompileOptions opts;
        opts.optimizationLevel = 2;
        opts.debugLevel = 1;
        opts.typeInfoLevel = 1;
        std::string compiled = Luau::compile(source, opts);
        auto compileMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - compileStart).count();
        auto inserted = m_bytecodeCache.emplace(key, std::move(compiled));
        geode::log::debug("luau compile [{}] {}ms", key, compileMs);
        return inserted.first->second;
    }

    bool Runtime::runScript(std::string_view src, std::string_view chunkName, int deadlineMs) {
        assertMainThread();

        std::string chunk(chunkName);
        std::string const& bytecode = getOrCompileBytecode(chunk, std::string(src));

        if (luau_load(m_state, chunk.c_str(), bytecode.data(), bytecode.size(), 0) != 0) {
            auto err = formatLuaError(chunk.c_str());
            geode::log::error("luau load failed {}", err);
            lua_pop(m_state, 1);
            return false;
        }

        if (m_codegenEnabled) {
            auto cgResult = Luau::CodeGen::compile(m_state, -1, Luau::CodeGen::CodeGen_ColdFunctions);
            if (cgResult.hasErrors()) {
                geode::log::warn("luau codegen [{}] partial: {}", chunk, Luau::CodeGen::toString(cgResult.result));
            }
        }

        auto execStart = std::chrono::steady_clock::now();
        bool ok = protectedCall(0, 0, chunk, deadlineMs);
        auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - execStart).count();

        if (!ok) {
            return false;
        }

        geode::log::debug("luau exec [{}] {}ms", chunk, execMs);
        return true;
    }

    bool Runtime::protectedCall(int nargs, int nresults, std::string_view context, int deadlineMs) {
        assertMainThread();
        int baseTop = lua_gettop(m_state) - nargs;
        if (nargs < 0 || baseTop < 1 || !lua_isfunction(m_state, baseTop)) {
            geode::log::error("[{}] luau protectedCall missing function", context);
            return false;
        }

        lua_getref(m_state, m_tracebackRef);
        lua_insert(m_state, -nargs - 2);
        int errfunc = lua_gettop(m_state) - nargs - 1;

        ScriptBudgetGuard budget(m_scriptBudgetMs, m_scriptDeadline, deadlineMs);

        int status = lua_pcall(m_state, nargs, nresults, errfunc);
        lua_remove(m_state, errfunc);

        if (status != 0) {
            std::string ctx(context);
            auto err = formatLuaError(ctx.c_str());
            geode::log::error("{}", err);
            lua_pop(m_state, 1);
            return false;
        }

        return true;
    }

    void Runtime::runOnMain(std::function<void()> fn) {
        geode::queueInMainThread(std::move(fn));
    }

    void Runtime::assertMainThread() const {
#ifndef NDEBUG
        assert(std::this_thread::get_id() == m_ownerThread);
#endif
    }

    void* Runtime::boundedAlloc(void* ud, void* ptr, size_t osize, size_t nsize) {
        auto* self = static_cast<Runtime*>(ud);
        if (nsize == 0) {
            if (ptr) {
                if (self) {
                    self->m_memoryUsage = osize > self->m_memoryUsage ? 0 : self->m_memoryUsage - osize;
                }
                std::free(ptr);
            }
            return nullptr;
        }
        if (self) {
            std::size_t delta = (nsize > osize) ? (nsize - osize) : 0;
            if (delta && self->m_memoryUsage + delta > self->m_memoryLimit) {
                // Returning null surfaces as a recoverable Luau OOM through pcall.
                return nullptr;
            }
        }
        void* out = std::realloc(ptr, nsize);
        if (!out) return nullptr;
        if (self) {
            if (osize > self->m_memoryUsage) {
                self->m_memoryUsage = nsize;
            } else {
                self->m_memoryUsage = self->m_memoryUsage - osize + nsize;
            }
        }
        return out;
    }

    void Runtime::interruptCallback(lua_State* L, int /*gc*/) {
        auto* self = static_cast<Runtime*>(lua_callbacks(L)->userdata);
        if (!self || self->m_scriptBudgetMs <= 0) return;
        if (std::chrono::steady_clock::now() >= self->m_scriptDeadline) {
            int budget = self->m_scriptBudgetMs;
            // Zero the budget so the error path does not re-enter this callback.
            self->m_scriptBudgetMs = 0;
            luaL_errorL(L, "luax: script exceeded %d ms budget", budget);
        }
    }

    void Runtime::panicCallback(lua_State* L, int errcode) {
        char const* message = lua_tostring(L, -1);
        geode::log::error("[lua:panic code={}] {}", errcode, message ? message : "unknown panic");
        std::abort();
    }
}
