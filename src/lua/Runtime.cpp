#include "Runtime.hpp"

#include "AllocatorAccounting.hpp"
#include "Binding.hpp"
#include "PathSandbox.hpp"
#include "Requirer.hpp"
#include "bindings/internal/Ref.hpp"

#include <Geode/Geode.hpp>
#include <fmt/format.h>
#include <Luau/CodeGen.h>
#include <Luau/Compiler.h>
#include <Luau/Require.h>
#include <lua.h>
#include <lualib.h>

#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace luax {
    namespace {
        constexpr char const kTracebackName[] = "luax:traceback";

        std::optional<Runtime>& runtimeStorage() {
            static std::optional<Runtime> runtime;
            return runtime;
        }

        std::atomic_bool& shuttingDownStorage() {
            static std::atomic_bool shuttingDown{false};
            return shuttingDown;
        }

        std::thread::id& mainThreadIdStorage() {
            static std::thread::id id;
            return id;
        }

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

        std::string valueToString(lua_State* L, int idx) {
            size_t len = 0;
            char const* text = luaL_tolstring(L, idx, &len);
            std::string out = text ? std::string(text, len) : std::string();
            lua_pop(L, 1);
            return out;
        }
    }

    Runtime::Runtime() : m_ownerThread(mainThreadIdStorage() == std::thread::id{} ? std::this_thread::get_id() : mainThreadIdStorage()) {
        m_state = lua_newstate(&Runtime::boundedAlloc, this);
        if (!m_state) {
            m_initError = "luau lua_newstate failed";
            setLastError(m_initError);
            m_status.store(imes::luauapi::RuntimeStatus::InitFailed, std::memory_order_release);
            geode::log::error("{}", m_initError);
            return;
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

        if (auto bindError = applyAllBindings(m_state)) {
            m_initError = "luau binding failed: " + *bindError;
            setLastError(m_initError);
            geode::log::error("{}", m_initError);
            m_requirer.reset();
            lua_close(m_state);
            m_state = nullptr;
            m_tracebackRef = 0;
            m_codegenEnabled = false;
            m_status.store(imes::luauapi::RuntimeStatus::InitFailed, std::memory_order_release);
            return;
        }

        // Mark globals as safe for fastcall, must be last.
        lua_pushvalue(m_state, LUA_GLOBALSINDEX);
        lua_setsafeenv(m_state, -1, true);
        lua_pop(m_state, 1);

        m_status.store(imes::luauapi::RuntimeStatus::Ready, std::memory_order_release);
        geode::log::info("luau runtime ready");
    }

    Runtime::~Runtime() {
        if (m_destroyed) return;
        m_destroyed = true;
        m_status.store(imes::luauapi::RuntimeStatus::NotReady, std::memory_order_release);

        for (auto it = m_shutdownHooks.rbegin(); it != m_shutdownHooks.rend(); ++it) {
            (*it)();
        }
        m_shutdownHooks.clear();

        if (m_state) {
            lua_close(m_state);
            m_state = nullptr;
        }

        clearLuaRetains();
        geode::log::info("luau runtime shutdown");
    }

    void Runtime::registerShutdownHook(std::function<void()> fn) {
        if (!assertMainThread()) return;
        if (fn) m_shutdownHooks.push_back(std::move(fn));
    }

    Runtime& Runtime::instance() {
        auto& runtime = runtimeStorage();
        if (!runtime) {
            runtime.emplace();
        }
        return *runtime;
    }

    bool Runtime::isInitialized() {
        auto& runtime = runtimeStorage();
        return runtime && runtime->ready();
    }

    Runtime* Runtime::getIfInitialized() {
        auto& runtime = runtimeStorage();
        if (!runtime) return nullptr;
        return &*runtime;
    }

    void Runtime::shutdown() {
        shuttingDownStorage().store(true, std::memory_order_release);
        auto& runtime = runtimeStorage();
        if (runtime) {
            runtime.reset();
        }
    }

    bool Runtime::isShuttingDown() {
        return shuttingDownStorage().load(std::memory_order_acquire);
    }

    void Runtime::setMainThreadId(std::thread::id id) {
        mainThreadIdStorage() = id;
    }

    bool Runtime::isMainThread() {
        auto const& id = mainThreadIdStorage();
        return id != std::thread::id{} && std::this_thread::get_id() == id;
    }

    lua_State* Runtime::state() {
        if (!assertMainThread()) return nullptr;
        if (!ready()) {
            if (m_lastError.empty()) {
                setLastError(m_initError.empty() ? "luau runtime not ready" : m_initError);
            }
            return nullptr;
        }
        return m_state;
    }

    bool Runtime::ready() const {
        return status() == imes::luauapi::RuntimeStatus::Ready && m_state;
    }

    imes::luauapi::RuntimeStatus Runtime::status() const {
        return m_status.load(std::memory_order_acquire);
    }

    Runtime::ScriptBudgetGuard::ScriptBudgetGuard(Runtime& runtime, int deadlineMs)
        : m_runtime(runtime),
          m_outermost(runtime.m_scriptBudgetDepth == 0),
          m_previousBudget(runtime.m_scriptBudgetMs),
          m_previousDeadline(runtime.m_scriptDeadline) {
        ++m_runtime.m_scriptBudgetDepth;
        if (m_outermost) {
            m_runtime.m_scriptBudgetMs = deadlineMs;
            if (deadlineMs > 0) {
                m_runtime.m_scriptDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(deadlineMs);
            }
        }
    }

    Runtime::ScriptBudgetGuard::~ScriptBudgetGuard() {
        if (m_runtime.m_scriptBudgetDepth > 0) {
            --m_runtime.m_scriptBudgetDepth;
        }
        if (m_outermost) {
            m_runtime.m_scriptBudgetMs = m_previousBudget;
            m_runtime.m_scriptDeadline = m_previousDeadline;
        }
    }

    void Runtime::setResourcesRoot(std::filesystem::path const& root) {
        if (!assertMainThread()) return;
        m_resourcesRoot = root;
        if (m_requirer) {
            m_requirer->setResourcesRoot(m_resourcesRoot);
        }
    }

    Runtime::ResourcesRootScope::ResourcesRootScope(Runtime& runtime, std::filesystem::path root)
        : m_runtime(runtime), m_previous(runtime.resourcesRoot()) {
        m_runtime.setResourcesRoot(root);
    }

    Runtime::ResourcesRootScope::~ResourcesRootScope() {
        m_runtime.setResourcesRoot(m_previous);
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

    void Runtime::setLastError(std::string error) {
        m_lastError = std::move(error);
    }

    std::string Runtime::compileSource(std::string_view source) {
        Luau::CompileOptions opts;
        opts.optimizationLevel = 2;
        opts.debugLevel = 1;
        opts.typeInfoLevel = 1;
        return Luau::compile(std::string(source), opts);
    }

    std::string const& Runtime::getOrCompileBytecode(std::string const& key, std::string const& source) {
        auto cached = m_bytecodeCache.find(key);
        if (cached != m_bytecodeCache.end()) {
            return cached->second;
        }

        if (m_bytecodeCache.size() >= kMaxBytecodeCacheEntries) {
            m_bytecodeCache.erase(m_bytecodeCache.begin());
        }

        auto compileStart = std::chrono::steady_clock::now();
        std::string compiled = compileSource(source);
        auto compileMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - compileStart).count();
        auto inserted = m_bytecodeCache.emplace(key, std::move(compiled));
        geode::log::debug("luau compile [{}] {}ms", key, compileMs);
        return inserted.first->second;
    }

    geode::Result<void> Runtime::runBytecode(std::string const& bytecode, std::string_view chunkName, int deadlineMs) {
        if (!assertMainThread()) {
            return geode::Err("luau runtime accessed off main thread");
        }
        if (!ready() || !m_state) {
            if (m_lastError.empty()) {
                return geode::Err(m_initError.empty() ? "luau runtime not ready" : m_initError);
            }
            return geode::Err(m_lastError);
        }

        clearLastError();

        std::string chunk(chunkName);
        if (luau_load(m_state, chunk.c_str(), bytecode.data(), bytecode.size(), 0) != 0) {
            auto err = formatLuaError(chunk.c_str());
            setLastError(err);
            geode::log::error("luau load failed {}", err);
            lua_pop(m_state, 1);
            return geode::Err(err);
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
            return geode::Err(m_lastError.empty() ? "luau script failed: " + chunk : m_lastError);
        }

        geode::log::debug("luau exec [{}] {}ms", chunk, execMs);
        return geode::Ok();
    }

    bool Runtime::runScript(std::string_view src, std::string_view chunkName, int deadlineMs) {
        if (!assertMainThread()) {
            setLastError("luau runtime accessed off main thread");
            return false;
        }
        if (!ready() || !m_state) {
            if (m_lastError.empty()) {
                setLastError(m_initError.empty() ? "luau runtime not ready" : m_initError);
            }
            return false;
        }
        if (src.size() > kMaxScriptBytes) {
            setLastError("script exceeds maximum size");
            geode::log::error("luau script exceeds maximum size: {}", chunkName);
            return false;
        }
        clearLastError();

        std::string chunk(chunkName);
        auto bytecodeKey = chunk;
        if (!m_resourcesRoot.empty()) {
            bytecodeKey = luax::normalizedPathString(m_resourcesRoot) + "|" + bytecodeKey;
        }
        bytecodeKey += "|";
        bytecodeKey += std::to_string(std::hash<std::string_view>{}(src));

        std::string const& bytecode = getOrCompileBytecode(bytecodeKey, std::string(src));

        if (luau_load(m_state, chunk.c_str(), bytecode.data(), bytecode.size(), 0) != 0) {
            auto err = formatLuaError(chunk.c_str());
            setLastError(err);
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
        if (!assertMainThread()) {
            setLastError("luau runtime accessed off main thread");
            return false;
        }
        if (!ready() || !m_state) {
            if (m_lastError.empty()) {
                setLastError(m_initError.empty() ? "luau runtime not ready" : m_initError);
            }
            return false;
        }
        int baseTop = lua_gettop(m_state) - nargs;
        if (nargs < 0 || baseTop < 1 || !lua_isfunction(m_state, baseTop)) {
            auto err = fmt::format("[{}] luau protectedCall missing function", context);
            setLastError(err);
            geode::log::error("{}", err);
            return false;
        }

        lua_getref(m_state, m_tracebackRef);
        lua_insert(m_state, -nargs - 2);
        int errfunc = lua_gettop(m_state) - nargs - 1;

        ScriptBudgetGuard budget(*this, deadlineMs);

        int status = lua_pcall(m_state, nargs, nresults, errfunc);
        lua_remove(m_state, errfunc);

        if (status != 0) {
            std::string ctx(context);
            auto err = formatLuaError(ctx.c_str());
            setLastError(err);
            geode::log::error("{}", err);
            lua_pop(m_state, 1);
            return false;
        }

        return true;
    }

    void Runtime::runOnMain(std::function<void()> fn) {
        geode::queueInMainThread(std::move(fn));
    }

    bool Runtime::assertMainThread() const {
        auto const& registered = mainThreadIdStorage();
        if (registered != std::thread::id{}) {
            if (std::this_thread::get_id() == registered) {
                return true;
            }
        } else if (std::this_thread::get_id() == m_ownerThread) {
            return true;
        }
        geode::log::error("luau runtime accessed off main thread");
        return false;
    }

    void* Runtime::boundedAlloc(void* ud, void* ptr, size_t osize, size_t nsize) {
        auto* self = static_cast<Runtime*>(ud);
        if (nsize == 0) {
            if (ptr) {
                if (self) {
                    self->m_memoryUsage = allocatorUsageAfterFree(self->m_memoryUsage, osize);
                }
                std::free(ptr);
            }
            return nullptr;
        }
        if (self) {
            if (!allocatorCanReallocate(self->m_memoryUsage, self->m_memoryLimit, osize, nsize)) {
                geode::log::error("luau memory limit exceeded ({} bytes)", self->m_memoryLimit);
                return nullptr;
            }
        }
        void* out = std::realloc(ptr, nsize);
        if (!out) return nullptr;
        if (self) {
            self->m_memoryUsage = allocatorUsageAfterReallocate(self->m_memoryUsage, osize, nsize);
        }
        return out;
    }

    void Runtime::interruptCallback(lua_State* L, int /*gc*/) {
        auto* self = static_cast<Runtime*>(lua_callbacks(L)->userdata);
        if (!self || self->m_scriptBudgetMs <= 0) return;
        if (std::chrono::steady_clock::now() >= self->m_scriptDeadline) {
            int budget = self->m_scriptBudgetMs;
            // Prevent re-entrance into this callback.
            self->m_scriptBudgetMs = 0;
            luaL_errorL(L, "luax: script exceeded %d ms budget", budget);
        }
    }

    void Runtime::panicCallback(lua_State* L, int errcode) {
        char const* message = lua_tostring(L, -1);
        geode::log::error("[lua:panic code={}] {}", errcode, message ? message : "unknown panic");

        auto* self = static_cast<Runtime*>(lua_callbacks(L)->userdata);
        if (self) {
            self->m_status.store(imes::luauapi::RuntimeStatus::Panicked, std::memory_order_release);
            self->setLastError(message ? message : "unknown lua panic");
        }
    }
}
