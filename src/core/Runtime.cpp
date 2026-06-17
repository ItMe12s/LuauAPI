#include "core/Runtime.hpp"

#include "bindings/geode/CurrentMod.hpp"
#include "core/AllocatorAccounting.hpp"
#include "core/BytecodeCacheAccounting.hpp"
#include "core/Loadstring.hpp"
#if !defined(LUAUAPI_HOST_TESTS)
    #include "framework/Binding.hpp"
    #include "framework/usertype/Ref.hpp"
    #include "require/Requirer.hpp"
#endif
#include "framework/stack/Stack.hpp"
#include "require/BytecodeCacheKey.hpp"
#include "require/PathSandbox.hpp"
#if defined(LUAUAPI_HOST_TESTS)
    #include "framework/usertype/Usertype.hpp"
#endif

#include <Geode/Geode.hpp>
#include <Luau/CodeGen.h>
#include <Luau/Compiler.h>
#if !defined(LUAUAPI_HOST_TESTS)
    #include <Luau/Require.h>
#endif
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <sstream>
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

        // #region agent log
        std::string jsonEscape(std::string_view value) {
            std::string out;
            out.reserve(value.size());
            for (char ch : value) {
                switch (ch) {
                    case '\\': out += "\\\\"; break;
                    case '"': out += "\\\""; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default: out += ch; break;
                }
            }
            return out;
        }

        std::string threadIdString(std::thread::id id) {
            std::ostringstream out;
            out << id;
            return out.str();
        }

        std::size_t threadIdHash(std::thread::id id) {
            return id == std::thread::id{} ? 0 : std::hash<std::thread::id>{}(id);
        }

        std::string boolJson(bool value) {
            return value ? "true" : "false";
        }

        std::string makeThreadProbeData(
            std::optional<std::thread::id> ownerThread, bool hasRuntime, bool ready
        ) {
            auto current = std::this_thread::get_id();
            auto registered = mainThreadIdStorage();
            bool registeredSet = registered != std::thread::id{};
            bool ownerSet = ownerThread.has_value();

            std::ostringstream data;
            data << "{\"thread\":\"" << jsonEscape(threadIdString(current)) << "\","
                 << "\"threadHash\":" << threadIdHash(current) << ","
                 << "\"registeredSet\":" << boolJson(registeredSet) << ","
                 << "\"registered\":\"" << jsonEscape(threadIdString(registered)) << "\","
                 << "\"registeredHash\":" << threadIdHash(registered) << ","
                 << "\"ownerSet\":" << boolJson(ownerSet) << ","
                 << "\"owner\":\""
                 << jsonEscape(threadIdString(ownerThread.value_or(std::thread::id{}))) << "\","
                 << "\"ownerHash\":" << threadIdHash(ownerThread.value_or(std::thread::id{})) << ","
                 << "\"hasRuntime\":" << boolJson(hasRuntime) << ","
                 << "\"ready\":" << boolJson(ready) << ","
                 << "\"isRegisteredThread\":" << boolJson(registeredSet && current == registered)
                 << ","
                 << "\"isOwnerThread\":" << boolJson(ownerSet && current == ownerThread.value());
            data << "}";
            return data.str();
        }

        void writeDebugThreadProbe(
            std::string_view runId, std::string_view hypothesisId, std::string_view location,
            std::string_view message, std::string_view dataJson
        ) {
            auto now = std::chrono::system_clock::now();
            auto timestamp =
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            auto micros =
                std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
            static std::atomic_uint64_t counter{0};

            std::ofstream out("debug-174777.log", std::ios::app);
            if (!out) return;
            out << "{\"sessionId\":\"174777\","
                << "\"id\":\"log_" << micros << "_" << counter.fetch_add(1) << "\","
                << "\"timestamp\":" << timestamp << ","
                << "\"location\":\"" << jsonEscape(location) << "\","
                << "\"message\":\"" << jsonEscape(message) << "\","
                << "\"data\":" << dataJson << ","
                << "\"runId\":\"" << jsonEscape(runId) << "\","
                << "\"hypothesisId\":\"" << jsonEscape(hypothesisId) << "\"}\n";
        }

        // #endregion

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

            std::string_view text(source);
            if (!text.empty() && (text.front() == '@' || text.front() == '=')) {
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
        // #region agent log
        writeDebugThreadProbe(
            "initial",
            "H1,H2",
            "src/core/Runtime.cpp:Runtime::Runtime",
            "runtime constructed and owner selected",
            makeThreadProbeData(m_ownerThread, true, false)
        );
        // #endregion
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
        auto hooks = std::move(m_shutdownHooks);
        for (auto it = hooks.rbegin(); it != hooks.rend(); ++it) {
            (*it)();
        }
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

    void Runtime::setShuttingDownForTests(bool value) {
        shuttingDownStorage().store(value, std::memory_order_release);
    }

    void Runtime::setStatusForTests(imes::luauapi::RuntimeStatus status) {
        m_status.store(status, std::memory_order_release);
    }

    void Runtime::runShutdownHooksForTests() {
        runShutdownHooks();
    }
#endif

    void Runtime::setMainThreadId(std::thread::id id) {
        mainThreadIdStorage() = id;
        // #region agent log
        debugThreadProbe(
            "initial",
            "H1,H2",
            "src/core/Runtime.cpp:Runtime::setMainThreadId",
            "registered main thread id"
        );
        // #endregion
    }

    bool Runtime::isMainThread() {
        auto const& id = mainThreadIdStorage();
        return id != std::thread::id{} && std::this_thread::get_id() == id;
    }

    void Runtime::debugThreadProbe(
        std::string_view runId, std::string_view hypothesisId, std::string_view location,
        std::string_view message
    ) {
        auto& runtime = runtimeStorage();
        std::optional<std::thread::id> owner;
        bool ready = false;
        if (runtime) {
            owner = runtime->m_ownerThread;
            ready = runtime->ready();
        }
        writeDebugThreadProbe(
            runId, hypothesisId, location, message, makeThreadProbeData(owner, runtime.has_value(), ready)
        );
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
        auto tmp = root;
        swapResourcesRoot(tmp);
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
            out.append(redactHostPaths(stackValueToString(L, i), resourcesRoot));
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
        auto bytecodeResult =
            self->getOrCompileBytecode(loadstringBytecodeKey(chunk, source), source);
        if (bytecodeResult.isErr()) {
            lua_pushnil(L);
            auto const& err = bytecodeResult.unwrapErr();
            lua_pushlstring(L, err.data(), err.size());
            return 2;
        }
        auto const& bytecode = bytecodeResult.unwrap().get();
        if (loadstringPushResult(L, chunk, bytecode) == LoadstringStatus::Failure) {
            return 2;
        }

        self->tryCompileLoadedChunk(L, chunk);

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

    geode::Result<void> Runtime::failWith(std::string error) {
        setLastError(std::move(error));
        return geode::Err(m_lastError);
    }

    geode::Result<void> Runtime::cachedError() const {
        return geode::Err(m_lastError);
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

    geode::Result<std::reference_wrapper<std::string const>> Runtime::getOrCompileBytecode(
        std::string const& key, std::string_view source
    ) {
        auto it = m_bytecodeIndex.find(key);
        if (it != m_bytecodeIndex.end()) {
            m_bytecodeLru.splice(m_bytecodeLru.begin(), m_bytecodeLru, it->second);
            return geode::Ok(std::cref(it->second->bytecode));
        }

        auto compileStart = std::chrono::steady_clock::now();
        std::string compiled = compileSource(source);
        auto compileMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - compileStart
        )
                             .count();

        if (!tryCacheCompiledBytecode(key, std::move(compiled), compileMs)) {
            m_bytecodeScratch.clear();
            if (m_lastError.empty()) {
                setLastError("luau compile failed");
            }
            return geode::Err(m_lastError);
        }

        if (m_bytecodeLru.empty() || m_bytecodeLru.front().key != key) {
            return geode::Ok(std::cref(m_bytecodeScratch));
        }
        return geode::Ok(std::cref(m_bytecodeLru.front().bytecode));
    }

    void Runtime::removeBytecodeCacheEntry(std::list<BytecodeCacheEntry>::iterator it) {
        std::size_t const entryBytes = bytecodeEntryBytes(it->bytecode);
        releaseExternalMemory(entryBytes);
        m_bytecodeCacheBytes = bytecodeCacheUsageAfterRemove(m_bytecodeCacheBytes, entryBytes);
        m_bytecodeIndex.erase(it->key);
        m_bytecodeLru.erase(it);
    }

    void Runtime::trimBytecodeCacheForInsert(std::size_t incomingBytes) {
        while (bytecodeCacheInsertNeedsEviction(
            m_bytecodeCacheBytes,
            kMaxBytecodeCacheBytes,
            incomingBytes,
            m_bytecodeIndex.size(),
            kMaxBytecodeCacheEntries,
            m_memoryUsage,
            m_memoryLimit
        )) {
            removeBytecodeCacheEntry(std::prev(m_bytecodeLru.end()));
        }
    }

    geode::Result<void> Runtime::ensureCallable(bool requireReady) {
        if (!assertMainThread()) {
            return failWith("luau runtime accessed off main thread");
        }
        if (isShuttingDown()) {
            return failWith("luau runtime shutting down");
        }
        if (requireReady) {
            if (!ready() || !m_state) {
                if (m_lastError.empty()) {
                    return failWith(m_initError.empty() ? "luau runtime not ready" : m_initError);
                }
                return cachedError();
            }
        }
        return geode::Ok();
    }

    void Runtime::tryCompileLoadedChunk(lua_State* L, std::string_view chunkName) {
        if (!m_codegenEnabled) return;
        auto cgResult = Luau::CodeGen::compile(L, -1, Luau::CodeGen::CodeGen_ColdFunctions);
        if (cgResult.hasErrors()) {
            geode::log::warn(
                "luau codegen [{}] partial: {}", chunkName, Luau::CodeGen::toString(cgResult.result)
            );
        }
    }

    geode::Result<void> Runtime::runScript(
        std::string_view src, std::string_view chunkName, int deadlineMs
    ) {
        if (auto guard = ensureCallable(); guard.isErr()) {
            return guard;
        }
        if (src.size() > kMaxScriptBytes) {
            geode::log::error("luau script exceeds maximum size: {}", chunkName);
            return failWith("script exceeds maximum size");
        }
        clearLastError();

        std::string chunk(chunkName);
        auto bytecodeKey = runScriptBytecodeKey(m_resourcesRoot, chunk, src);

        auto bytecodeResult = getOrCompileBytecode(bytecodeKey, src);
        if (bytecodeResult.isErr()) {
            geode::log::error("luau compile failed [{}] {}", chunk, bytecodeResult.unwrapErr());
            return geode::Err(bytecodeResult.unwrapErr());
        }
        auto const& bytecode = bytecodeResult.unwrap().get();

        if (luau_load(m_state, chunk.c_str(), bytecode.data(), bytecode.size(), 0) != 0) {
            auto err = formatLuaError(chunk.c_str());
            geode::log::error("luau load failed {}", err);
            lua_pop(m_state, 1);
            return failWith(std::move(err));
        }

        tryCompileLoadedChunk(m_state, chunk);

        auto execStart = std::chrono::steady_clock::now();
        auto callResult = protectedCall(m_state, 0, 0, chunk, deadlineMs);
        auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - execStart
        )
                          .count();

        if (callResult.isErr()) {
            return callResult;
        }

        geode::log::debug("luau exec [{}] {}ms", chunk, execMs);
        return geode::Ok();
    }

    geode::Result<void> Runtime::protectedCallImpl(
        lua_State* invokeL, int nargs, int nresults, std::string_view context,
        ProtectedCallPolicy policy, int deadlineMs
    ) {
        if (auto guard = ensureCallable(policy == ProtectedCallPolicy::WithBudget); guard.isErr()) {
            return guard;
        }
        if (policy == ProtectedCallPolicy::TracebackOnly) {
            if (status() == imes::luauapi::RuntimeStatus::Panicked) {
                return m_lastError.empty() ? failWith("luau runtime panicked") : cachedError();
            }
            if (!m_state || !m_tracebackRef) {
                if (m_lastError.empty()) {
                    return failWith(m_initError.empty() ? "luau runtime not available" : m_initError);
                }
                return cachedError();
            }
        }

        if (!invokeL) {
            invokeL = m_state;
        }

        int const mainSavedTop = lua_gettop(m_state);
        bool const crossState = invokeL != m_state;

        if (crossState) {
            int const invokeTop = lua_gettop(invokeL);
            int const funcIdx = invokeTop - nargs;
            if (nargs < 0 || funcIdx < 1 || !lua_isfunction(invokeL, funcIdx)) {
                auto err = fmt::format("[{}] luau protectedCall missing function", context);
                geode::log::error("{}", err);
                return failWith(std::move(err));
            }
            lua_xmove(invokeL, m_state, nargs + 1);
        }
        else {
            int baseTop = lua_gettop(m_state) - nargs;
            if (nargs < 0 || baseTop < 1 || !lua_isfunction(m_state, baseTop)) {
                auto err = fmt::format("[{}] luau protectedCall missing function", context);
                geode::log::error("{}", err);
                return failWith(std::move(err));
            }
        }

        lua_getref(m_state, m_tracebackRef);
        lua_insert(m_state, -nargs - 2);
        int errfunc = lua_gettop(m_state) - nargs - 1;

        std::optional<ScriptBudgetGuard> budget;
        if (policy == ProtectedCallPolicy::WithBudget) {
            budget.emplace(*this, deadlineMs);
        }

        int callStatus = lua_pcall(m_state, nargs, nresults, errfunc);
        lua_remove(m_state, errfunc);

        if (callStatus != 0) {
            std::string ctx(context);
            auto err = formatLuaError(ctx.c_str());
            geode::log::error("{}", err);
            lua_pop(m_state, 1);
            return failWith(std::move(err));
        }

        if (crossState) {
            int resultCount = nresults;
            if (nresults == LUA_MULTRET) {
                resultCount = lua_gettop(m_state) - mainSavedTop;
            }
            if (resultCount > 0) {
                lua_xmove(m_state, invokeL, resultCount);
            }
        }

        return geode::Ok();
    }

    geode::Result<void> Runtime::protectedCall(
        lua_State* L, int nargs, int nresults, std::string_view context, int deadlineMs
    ) {
        return protectedCallImpl(
            L, nargs, nresults, context, ProtectedCallPolicy::WithBudget, deadlineMs
        );
    }

    geode::Result<void> Runtime::protectedCallWithTraceback(
        lua_State* L, int nargs, int nresults, std::string_view context
    ) {
        return protectedCallImpl(
            L, nargs, nresults, context, ProtectedCallPolicy::TracebackOnly, kDefaultScriptDeadlineMs
        );
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
        static std::atomic_bool s_loggedAssertFailure{false};
        bool expected = false;
        if (s_loggedAssertFailure.compare_exchange_strong(expected, true)) {
            // #region agent log
            debugThreadProbe(
                "initial",
                "H2,H4,H5",
                "src/core/Runtime.cpp:Runtime::assertMainThread",
                "first runtime main-thread assertion failure"
            );
            // #endregion
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
