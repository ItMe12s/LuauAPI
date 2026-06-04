#include "Runtime.hpp"

#include "AllocatorAccounting.hpp"
#include "BytecodeCacheAccounting.hpp"
#include "Loadstring.hpp"
#include "lua/bindings/geode/CurrentMod.hpp"
#if !defined(LUAUAPI_HOST_TESTS)
    #include "lua/bindings/Binding.hpp"
    #include "lua/bindings/framework/Ref.hpp"
    #include "lua/module/Requirer.hpp"
#endif
#include "lua/module/BytecodeCacheKey.hpp"
#include "lua/module/PathSandbox.hpp"
#if defined(LUAUAPI_HOST_TESTS)
    #include "lua/bindings/framework/Usertype.hpp"
#endif

#include <Geode/Geode.hpp>
#include <Luau/CodeGen.h>
#include <Luau/Compiler.h>
#if !defined(LUAUAPI_HOST_TESTS)
    #include <Luau/Require.h>
#endif
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <lua.h>
#include <lualib.h>
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

            void dismiss() {
                m_active = false;
            }

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

        bool iequalsPrefix(std::string_view haystack, std::string_view needle) {
            if (haystack.size() < needle.size()) return false;
            for (std::size_t i = 0; i < needle.size(); ++i) {
                auto a = static_cast<unsigned char>(haystack[i]);
                auto b = static_cast<unsigned char>(needle[i]);
                if (std::tolower(a) != std::tolower(b)) return false;
            }
            return true;
        }

        std::string formatDebugSource(char const* source, std::filesystem::path const& resourcesRoot) {
            if (!source || !source[0]) return {};
            if (source[0] == '@') return source;

            std::string_view text(source);
            if (!text.empty() && text.front() == '=') {
                text.remove_prefix(1);
            }

            std::filesystem::path filePath(text);
            if (filePath.empty()) return source;

            if (!resourcesRoot.empty() && filePath.is_absolute()) {
                auto rootResult = canonicalRoot(resourcesRoot);
                if (rootResult.isOk()) {
                    std::error_code ec;
                    auto resolved = std::filesystem::weakly_canonical(filePath, ec);
                    if (!ec && pathInsideRoot(resolved, rootResult.unwrap())) {
                        auto rel = resolved.lexically_relative(rootResult.unwrap());
                        return "@" + normalizedPathString(rel);
                    }
                }
            }

            if (filePath.is_absolute()) {
                auto name = filePath.filename();
                if (!name.empty()) {
                    return "@" + normalizedPathString(name);
                }
            }

            return source;
        }

        std::size_t findRootPrefix(std::string const& text, std::string_view rootText, std::size_t pos) {
            if (rootText.empty() || pos >= text.size()) return std::string::npos;
#if defined(_WIN32)
            for (std::size_t scan = pos; scan + rootText.size() <= text.size(); ++scan) {
                if (iequalsPrefix(std::string_view(text).substr(scan), rootText)) {
                    return scan;
                }
            }
            return std::string::npos;
#else
            return text.find(rootText, pos);
#endif
        }

        void replaceRootPrefix(std::string& text, std::string_view rootText) {
            if (rootText.empty()) return;
            std::size_t pos = 0;
            while (pos < text.size()) {
                std::size_t const found = findRootPrefix(text, rootText, pos);
                if (found == std::string::npos) break;

                std::size_t tail = found + rootText.size();
                while (tail < text.size() && (text[tail] == '/' || text[tail] == '\\')) {
                    ++tail;
                }
                text.replace(found, tail - found, "@");
                pos = found + 1;
            }
        }

        std::string redactHostPaths(std::string_view text, std::filesystem::path const& resourcesRoot) {
            std::string out(text);
            if (resourcesRoot.empty()) return out;

            auto rootResult = canonicalRoot(resourcesRoot);
            if (rootResult.isErr()) return out;

            auto rootText = filesystemPathString(rootResult.unwrap());
            replaceRootPrefix(out, rootText);

            auto genericRoot = normalizedPathString(rootResult.unwrap());
            if (genericRoot != rootText) {
                replaceRootPrefix(out, genericRoot);
            }

            return out;
        }
    } // namespace

    Runtime::Runtime() :
        m_ownerThread(
            mainThreadIdStorage() == std::thread::id{} ? std::this_thread::get_id() :
                                                         mainThreadIdStorage()
        ) {
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
        }
        else {
            geode::log::info("luau codegen not supported on this platform, using interpreter");
        }

        installTraceback();
        installPrint();
        installLoadstring();

        auto* cb = lua_callbacks(m_state);
        cb->interrupt = &Runtime::interruptCallback;
        cb->panic = &Runtime::panicCallback;
        cb->userdata = this;

#if !defined(LUAUAPI_HOST_TESTS)
        m_requirer = std::make_unique<Requirer>(*this);
        luaopen_require(m_state, &Requirer::initConfig, m_requirer.get());

        if (auto bindError = applyAllBindings(m_state)) {
            m_initError = "luau binding failed: " + *bindError;
            setLastError(m_initError);
            geode::log::error("{}", m_initError);
            ++m_generation;
            runShutdownHooks();
            m_requirer.reset();
            lua_close(m_state);
            m_state = nullptr;
            m_tracebackRef = 0;
            m_codegenEnabled = false;
            m_status.store(imes::luauapi::RuntimeStatus::InitFailed, std::memory_order_release);
            return;
        }
#endif

        lua_pushvalue(m_state, LUA_GLOBALSINDEX);
        lua_setsafeenv(m_state, -1, true);
        lua_pop(m_state, 1);

        m_status.store(imes::luauapi::RuntimeStatus::Ready, std::memory_order_release);
        geode::log::info("luau runtime ready");
    }

    Runtime::~Runtime() {
        if (m_destroyed) return;
        m_destroyed = true;
        ++m_generation;
        m_status.store(imes::luauapi::RuntimeStatus::NotReady, std::memory_order_release);

        runShutdownHooks();

        if (m_state) {
            lua_close(m_state);
            m_state = nullptr;
        }

#if !defined(LUAUAPI_HOST_TESTS)
        clearLuaRetains();
#endif
        geode::log::info("luau runtime shutdown");
    }

    void Runtime::runShutdownHooks() {
        for (auto it = m_shutdownHooks.rbegin(); it != m_shutdownHooks.rend(); ++it) {
            (*it)();
        }
        m_shutdownHooks.clear();
    }

    void Runtime::registerShutdownHook(std::function<void()> fn) {
        if (!assertMainThread()) return;
        if (fn) m_shutdownHooks.push_back(std::move(fn));
    }

    Runtime* Runtime::getOrCreate() {
        if (isShuttingDown()) return nullptr;
        auto& runtime = runtimeStorage();
        if (!runtime) {
            runtime.emplace();
        }
        return &*runtime;
    }

    bool Runtime::isInitialized() {
        if (isShuttingDown()) return false;
        auto& runtime = runtimeStorage();
        return runtime && runtime->ready();
    }

    Runtime* Runtime::getIfInitialized() {
        if (isShuttingDown()) return nullptr;
        auto& runtime = runtimeStorage();
        if (!runtime) return nullptr;
        return &*runtime;
    }

    void Runtime::shutdown() {
        shuttingDownStorage().store(true, std::memory_order_release);
        invalidateCurrentModCache();
        auto& runtime = runtimeStorage();
        if (runtime) {
            runtime.reset();
        }
    }

    bool Runtime::isShuttingDown() {
        return shuttingDownStorage().load(std::memory_order_acquire);
    }

#if defined(LUAUAPI_HOST_TESTS)
    void Runtime::resetForTests() {
        shuttingDownStorage().store(false, std::memory_order_release);
        invalidateCurrentModCache();
        detail::UsertypeRegistry::get().resetForTests();
        auto& runtime = runtimeStorage();
        runtime.reset();
    }

    void Runtime::setStatusForTests(imes::luauapi::RuntimeStatus status) {
        m_status.store(status, std::memory_order_release);
    }
#endif

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

    Runtime::ScriptBudgetGuard::ScriptBudgetGuard(Runtime& runtime, int deadlineMs) :
        m_runtime(runtime), m_outermost(runtime.m_scriptBudgetDepth == 0),
        m_previousBudget(runtime.m_scriptBudgetMs), m_previousDeadline(runtime.m_scriptDeadline) {
        ++m_runtime.m_scriptBudgetDepth;
        if (m_outermost) {
            m_runtime.m_scriptBudgetMs = deadlineMs;
            if (deadlineMs > 0) {
                m_runtime.m_scriptDeadline =
                    std::chrono::steady_clock::now() + std::chrono::milliseconds(deadlineMs);
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
        if (m_resourcesRoot == root) return;
        m_resourcesRoot = root;
#if !defined(LUAUAPI_HOST_TESTS)
        if (m_requirer) {
            m_requirer->setResourcesRoot(m_resourcesRoot);
        }
#endif
    }

    void Runtime::swapResourcesRoot(std::filesystem::path& root) {
        if (!assertMainThread()) return;
        m_resourcesRoot.swap(root);
#if !defined(LUAUAPI_HOST_TESTS)
        if (m_requirer) {
            m_requirer->setResourcesRoot(m_resourcesRoot);
        }
#endif
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

    void Runtime::installLoadstring() {
        lua_pushcfunction(m_state, &Runtime::luaLoadstring, "loadstring");
        lua_setglobal(m_state, "loadstring");
    }

    int Runtime::luaPrint(lua_State* L) {
        StackGuard stack(L);
        int argc = lua_gettop(L);
        std::string out;

        auto* self = static_cast<Runtime*>(lua_callbacks(L)->userdata);
        std::filesystem::path const& resourcesRoot =
            self ? self->resourcesRoot() : std::filesystem::path{};

        for (int i = 1; i <= argc; ++i) {
            if (i > 1) out.push_back('\t');
            out.append(redactHostPaths(valueToString(L, i), resourcesRoot));
        }

        geode::log::info("[lua] {}", out);
        return 0;
    }

    int Runtime::luaLoadstring(lua_State* L) {
        auto* self = static_cast<Runtime*>(lua_callbacks(L)->userdata);
        if (!self) {
            lua_pushnil(L);
            lua_pushliteral(L, "luau runtime not ready");
            return 2;
        }

        size_t sourceLen = 0;
        char const* sourceData = luaL_checklstring(L, 1, &sourceLen);
        std::string_view source(sourceData, sourceLen);
        if (source.size() > kMaxScriptBytes) {
            lua_pushnil(L);
            lua_pushliteral(L, "script exceeds maximum size");
            return 2;
        }

        char const* chunkData = luaL_optstring(L, 2, "=loadstring");
        std::string_view chunk(chunkData ? chunkData : "=loadstring");
        bool ok = false;
        auto const& bytecode =
            self->getOrCompileBytecode(loadstringBytecodeKey(chunk, source), source, ok);
        if (!ok) {
            lua_pushnil(L);
            lua_pushlstring(L, self->lastError().data(), self->lastError().size());
            return 2;
        }
        if (loadstringPushResult(L, chunk, bytecode) == LoadstringStatus::Failure) {
            return 2;
        }

        if (self->m_codegenEnabled) {
            auto cgResult = Luau::CodeGen::compile(L, -1, Luau::CodeGen::CodeGen_ColdFunctions);
            if (cgResult.hasErrors()) {
                geode::log::warn(
                    "luau codegen [{}] partial: {}", chunk, Luau::CodeGen::toString(cgResult.result)
                );
            }
        }

        return 1;
    }

    int Runtime::luaTraceback(lua_State* L) {
        auto* self = static_cast<Runtime*>(lua_callbacks(L)->userdata);
        std::filesystem::path const& resourcesRoot =
            self ? self->resourcesRoot() : std::filesystem::path{};

        char const* msg = lua_tostring(L, 1);
        std::string out;
        if (msg) {
            out = redactHostPaths(msg, resourcesRoot);
        }
        else {
            out.assign("(non-string error)");
        }

        lua_Debug ar;
        out.append("\n  stack:");
        for (int level = 0; lua_getinfo(L, level, "sln", &ar); ++level) {
            out.append("\n    ");
            if (ar.source) {
                out.append(formatDebugSource(ar.short_src, resourcesRoot));
            }
            if (ar.currentline > 0) {
                out.append(":");
                out.append(geode::utils::numToString(ar.currentline));
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
        return redactHostPaths(out, m_resourcesRoot);
    }

    void Runtime::setLastError(std::string error) {
        m_lastError = redactHostPaths(error, m_resourcesRoot);
    }

    std::string Runtime::compileSource(std::string_view source) {
        Luau::CompileOptions opts;
        opts.optimizationLevel = 2;
        opts.debugLevel = 1;
        opts.typeInfoLevel = 1;
        return Luau::compile(std::string(source), opts);
    }

    bool Runtime::reserveExternalMemory(std::size_t bytes) {
        if (!memoryBudgetAllows(m_memoryUsage, m_memoryLimit, bytes)) {
            return false;
        }
        m_memoryUsage += bytes;
        return true;
    }

    void Runtime::releaseExternalMemory(std::size_t bytes) {
        m_memoryUsage = allocatorUsageAfterFree(m_memoryUsage, bytes);
    }

    bool Runtime::tryCacheCompiledBytecode(
        std::string const& key, std::string compiled, long long compileMs
    ) {
        if (!compileTimeWithinBudget(compileMs, kMaxCompileDeadlineMs)) {
            setLastError(fmt::format("luau compile exceeded {} ms budget", kMaxCompileDeadlineMs));
            geode::log::warn(
                "luau compile [{}] exceeded {}ms budget ({}ms)", key, kMaxCompileDeadlineMs, compileMs
            );
            return false;
        }

        std::size_t const entryBytes = bytecodeEntryBytes(compiled);
        if (entryBytes > kMaxBytecodeCacheBytes) {
            m_bytecodeScratch = std::move(compiled);
            geode::log::debug("luau compile [{}] {}ms (not cached)", key, compileMs);
            return true;
        }

        trimBytecodeCacheForInsert(entryBytes);
        while (!memoryBudgetAllows(m_memoryUsage, m_memoryLimit, entryBytes) &&
               !m_bytecodeLru.empty()) {
            removeBytecodeCacheEntry(std::prev(m_bytecodeLru.end()));
        }
        if (!reserveExternalMemory(entryBytes)) {
            setLastError("luau memory limit exceeded");
            geode::log::error("luau memory limit exceeded ({} bytes)", m_memoryLimit);
            return false;
        }

        m_bytecodeLru.push_front({key, std::move(compiled)});
        m_bytecodeIndex[key] = m_bytecodeLru.begin();
        m_bytecodeCacheBytes = bytecodeCacheUsageAfterInsert(m_bytecodeCacheBytes, entryBytes);
        geode::log::debug("luau compile [{}] {}ms", key, compileMs);
        return true;
    }

    std::string const& Runtime::getOrCompileBytecode(
        std::string const& key, std::string_view source, bool& ok
    ) {
        ok = true;
        auto it = m_bytecodeIndex.find(key);
        if (it != m_bytecodeIndex.end()) {
            m_bytecodeLru.splice(m_bytecodeLru.begin(), m_bytecodeLru, it->second);
            return it->second->bytecode;
        }

        auto compileStart = std::chrono::steady_clock::now();
        std::string compiled = compileSource(source);
        auto compileMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - compileStart
        )
                             .count();

        if (!tryCacheCompiledBytecode(key, std::move(compiled), compileMs)) {
            ok = false;
            m_bytecodeScratch.clear();
            return m_bytecodeScratch;
        }

        if (m_bytecodeLru.empty() || m_bytecodeLru.front().key != key) {
            return m_bytecodeScratch;
        }
        return m_bytecodeLru.front().bytecode;
    }

    void Runtime::removeBytecodeCacheEntry(std::list<BytecodeCacheEntry>::iterator it) {
        std::size_t const entryBytes = bytecodeEntryBytes(it->bytecode);
        releaseExternalMemory(entryBytes);
        m_bytecodeCacheBytes = bytecodeCacheUsageAfterRemove(m_bytecodeCacheBytes, entryBytes);
        m_bytecodeIndex.erase(it->key);
        m_bytecodeLru.erase(it);
    }

    void Runtime::trimBytecodeCacheForInsert(std::size_t incomingBytes) {
        while (bytecodeCacheNeedsEviction(
            m_bytecodeCacheBytes, kMaxBytecodeCacheBytes, incomingBytes, m_bytecodeIndex.size(), kMaxBytecodeCacheEntries
        )) {
            removeBytecodeCacheEntry(std::prev(m_bytecodeLru.end()));
        }
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
        auto bytecodeKey = runScriptBytecodeKey(m_resourcesRoot, chunk, src);

        bool compileOk = false;
        std::string const& bytecode = getOrCompileBytecode(bytecodeKey, src, compileOk);
        if (!compileOk) {
            geode::log::error("luau compile failed [{}] {}", chunk, lastError());
            return false;
        }

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
                geode::log::warn(
                    "luau codegen [{}] partial: {}", chunk, Luau::CodeGen::toString(cgResult.result)
                );
            }
        }

        auto execStart = std::chrono::steady_clock::now();
        bool ok = protectedCall(0, 0, chunk, deadlineMs);
        auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - execStart
        )
                          .count();

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

    bool Runtime::protectedCallWithTraceback(int nargs, int nresults, std::string_view context) {
        if (!assertMainThread()) {
            setLastError("luau runtime accessed off main thread");
            return false;
        }
        if (!m_state || !m_tracebackRef) {
            if (m_lastError.empty()) {
                setLastError(m_initError.empty() ? "luau runtime not available" : m_initError);
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

    bool Runtime::assertMainThread() const {
        auto const& registered = mainThreadIdStorage();
        if (registered != std::thread::id{}) {
            if (std::this_thread::get_id() == registered) {
                return true;
            }
        }
        else if (std::this_thread::get_id() == m_ownerThread) {
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
} // namespace luax
