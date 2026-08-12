#include "core/Runtime.hpp"
#include "diagnostics/BoundaryRecorder.hpp"
#include "framework/stack/Stack.hpp"
#include "require/PathSandbox.hpp"

#if !defined(LUAUAPI_HOST_TESTS)
    #define GEODE_DEFINE_EVENT_EXPORTS
#endif
#include <LuauAPI.hpp>

#if !defined(LUAUAPI_HOST_TESTS)
    #include <Geode/utils/async.hpp>
#endif
#include <Geode/utils/string.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace imes::luauapi {
    geode::Result<void> resolveAsyncMainThreadResult(std::optional<geode::Result<void>> const& result);
}

namespace {
    namespace native_detail = imes::luauapi::detail;

    constexpr std::size_t kMaxNativeFunctionBytes = 64;

    enum class NativeLeafKind {
        Function,
        Value,
    };

    struct NativeRegistrationRequest {
        NativeLeafKind kind = NativeLeafKind::Value;
        std::string providerId;
        std::vector<std::string> segments;
        std::string qualifiedName;
        native_detail::NativeInvoker invoker = nullptr;
        std::vector<std::byte> functionBytes;
        native_detail::NativeValue value;
        std::string valueString;
        std::string error;
    };

    struct NativeClosureHeader {
        native_detail::NativeInvoker invoker = nullptr;
        std::size_t functionSize = 0;
        std::size_t nameOffset = 0;
    };

    static_assert(std::is_trivially_copyable_v<NativeClosureHeader>);

    struct NativeCallState {
        lua_State* state = nullptr;
        int argumentTop = 0;
        std::uint32_t pushed = 0;
        std::string error;
    };

    NativeCallState& nativeCallState(void* state) {
        return *static_cast<NativeCallState*>(state);
    }

    std::uint32_t nativeArgumentCount(void* state) noexcept {
        int count = nativeCallState(state).argumentTop;
        return count > 0 ? static_cast<std::uint32_t>(count) : 0;
    }

    native_detail::NativeValueKind nativeArgumentKind(void* state, std::uint32_t index) noexcept {
        auto& call = nativeCallState(state);
        int luaIndex = static_cast<int>(index) + 1;
        if (luaIndex > call.argumentTop || lua_isnil(call.state, luaIndex)) {
            return native_detail::NativeValueKind::Nil;
        }

        switch (lua_type(call.state, luaIndex)) {
            case LUA_TBOOLEAN: return native_detail::NativeValueKind::Boolean;
            case LUA_TINTEGER: return native_detail::NativeValueKind::Integer;
            case LUA_TNUMBER: return native_detail::NativeValueKind::Number;
            case LUA_TSTRING: return native_detail::NativeValueKind::String;
            default: return native_detail::NativeValueKind::Unsupported;
        }
    }

    std::uint8_t nativeReadBoolean(void* state, std::uint32_t index, std::uint8_t* out) noexcept {
        auto& call = nativeCallState(state);
        int luaIndex = static_cast<int>(index) + 1;
        if (!out || luaIndex > call.argumentTop || lua_type(call.state, luaIndex) != LUA_TBOOLEAN) {
            return false;
        }
        *out = lua_toboolean(call.state, luaIndex) != 0 ? 1 : 0;
        return true;
    }

    std::uint8_t nativeReadInteger(void* state, std::uint32_t index, std::int64_t* out) noexcept {
        auto& call = nativeCallState(state);
        int luaIndex = static_cast<int>(index) + 1;
        if (!out || luaIndex > call.argumentTop) return false;

        if (lua_isinteger64(call.state, luaIndex)) {
            int isInteger = 0;
            auto value = lua_tointeger64(call.state, luaIndex, &isInteger);
            if (!isInteger) return false;
            *out = value;
            return true;
        }

        if (lua_type(call.state, luaIndex) != LUA_TNUMBER) return false;
        double value = lua_tonumber(call.state, luaIndex);
        constexpr double kInt64Limit = 9223372036854775808.0;
        if (!std::isfinite(value) || std::trunc(value) != value || value < -kInt64Limit ||
            value >= kInt64Limit) {
            return false;
        }
        *out = static_cast<std::int64_t>(value);
        return true;
    }

    std::uint8_t nativeReadNumber(void* state, std::uint32_t index, double* out) noexcept {
        auto& call = nativeCallState(state);
        int luaIndex = static_cast<int>(index) + 1;
        if (!out || luaIndex > call.argumentTop) return false;

        if (lua_isinteger64(call.state, luaIndex)) {
            int isInteger = 0;
            auto value = lua_tointeger64(call.state, luaIndex, &isInteger);
            if (!isInteger || value < native_detail::kLuauMinSafeInteger ||
                value > native_detail::kLuauMaxSafeInteger)
                return false;
            *out = static_cast<double>(value);
            return std::isfinite(*out);
        }
        if (lua_type(call.state, luaIndex) != LUA_TNUMBER) return false;
        *out = lua_tonumber(call.state, luaIndex);
        return std::isfinite(*out);
    }

    std::uint8_t nativeReadString(
        void* state, std::uint32_t index, char const** data, std::uint64_t* size
    ) noexcept {
        auto& call = nativeCallState(state);
        int luaIndex = static_cast<int>(index) + 1;
        if (!data || !size || luaIndex > call.argumentTop ||
            lua_type(call.state, luaIndex) != LUA_TSTRING) {
            return false;
        }

        size_t length = 0;
        char const* bytes = lua_tolstring(call.state, luaIndex, &length);
        if (!bytes) return false;
        *data = bytes;
        *size = static_cast<std::uint64_t>(length);
        return true;
    }

    void nativePushNil(void* state) {
        auto& call = nativeCallState(state);
        lua_pushnil(call.state);
        ++call.pushed;
    }

    void nativePushBoolean(void* state, std::uint8_t value) {
        auto& call = nativeCallState(state);
        lua_pushboolean(call.state, value);
        ++call.pushed;
    }

    void nativePushInteger(void* state, std::int64_t value) {
        auto& call = nativeCallState(state);
        if (value >= native_detail::kLuauMinSafeInteger && value <= native_detail::kLuauMaxSafeInteger)
            lua_pushnumber(call.state, static_cast<double>(value));
        else lua_pushinteger64(call.state, value);
        ++call.pushed;
    }

    void nativePushNumber(void* state, double value) {
        auto& call = nativeCallState(state);
        lua_pushnumber(call.state, value);
        ++call.pushed;
    }

    void nativePushString(void* state, char const* data, std::uint64_t size) {
        auto& call = nativeCallState(state);
        if (size > (std::numeric_limits<std::size_t>::max)()) {
            call.error = "native callback string is too large";
            return;
        }
        lua_pushlstring(call.state, data ? data : "", static_cast<std::size_t>(size));
        ++call.pushed;
    }

    void nativeSetError(void* state, char const* data, std::uint64_t size) {
        auto& call = nativeCallState(state);
        if (size > (std::numeric_limits<std::size_t>::max)()) {
            call.error = "native callback error is too large";
            return;
        }
        call.error.assign(data ? data : "", static_cast<std::size_t>(size));
    }

    native_detail::NativeCallOps const kNativeCallOps{
        &nativeArgumentCount,
        &nativeArgumentKind,
        &nativeReadBoolean,
        &nativeReadInteger,
        &nativeReadNumber,
        &nativeReadString,
        &nativePushNil,
        &nativePushBoolean,
        &nativePushInteger,
        &nativePushNumber,
        &nativePushString,
        &nativeSetError,
    };

    int nativeFunctionTrampoline(lua_State* L) {
        auto* data = static_cast<std::byte*>(lua_touserdata(L, lua_upvalueindex(1)));
        if (!data) {
            luaL_error(L, "native function descriptor is missing");
            return 0;
        }

        NativeClosureHeader header;
        std::memcpy(&header, data, sizeof(header));
        if (!header.invoker || header.functionSize == 0 ||
            header.nameOffset < sizeof(NativeClosureHeader) + header.functionSize) {
            luaL_error(L, "native function descriptor is invalid");
            return 0;
        }

        char const* qualifiedName = reinterpret_cast<char const*>(data + header.nameOffset);
        std::string error;
        int resultCount = -1;
        int const argumentTop = lua_gettop(L);

        {
            auto boundary = luax::diag::recordBindingEntry(
                L, qualifiedName, luax::diag::BoundaryKind::NativeFunction
            );
            NativeCallState state{L, argumentTop, 0, {}};
            native_detail::NativeCall call{&state, &kNativeCallOps};
            resultCount =
                header.invoker(data + sizeof(NativeClosureHeader), header.functionSize, &call);

            if (!state.error.empty()) {
                error = std::move(state.error);
            }
            else if (resultCount < 0) {
                error = "native callback failed";
            }
            else if (static_cast<std::uint32_t>(resultCount) != state.pushed) {
                error = "native callback returned an invalid result count";
                resultCount = -1;
            }
        }

        if (!error.empty() || resultCount < 0) {
            lua_settop(L, argumentTop);
            std::string message =
                "native function " + std::string(qualifiedName) + " failed: " + error;
            luaL_error(L, "%s", message.c_str());
            return 0;
        }
        return resultCount;
    }

    bool isIdentifierSegment(std::string_view segment) {
        if (segment.empty()) return false;
        auto first = static_cast<unsigned char>(segment.front());
        if (!((first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z') || first == '_')) {
            return false;
        }
        for (unsigned char ch : segment.substr(1)) {
            if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                  (ch >= '0' && ch <= '9') || ch == '_')) {
                return false;
            }
        }
        return true;
    }

    std::string quotedKey(std::string_view key) {
        std::string result;
        result.reserve(key.size() + 2);
        result.push_back('"');
        for (char ch : key) {
            if (ch == '\\' || ch == '"') result.push_back('\\');
            result.push_back(ch);
        }
        result.push_back('"');
        return result;
    }

    std::string qualifiedName(std::string_view providerId, std::vector<std::string> const& segments) {
        std::string result = "_G[" + quotedKey(providerId) + "]";
        for (auto const& segment : segments) {
            if (isIdentifierSegment(segment)) {
                result.push_back('.');
                result.append(segment);
            }
            else {
                result.push_back('[');
                result.append(quotedKey(segment));
                result.push_back(']');
            }
        }
        return result;
    }

    geode::Result<std::vector<std::string>> parseNativePath(std::string_view path) {
        if (path.empty()) return geode::Err("native registration path is empty");
        if (path.find('\0') != std::string_view::npos) {
            return geode::Err("native registration path contains NUL");
        }

        auto segments = geode::utils::string::split(path, ".");
        for (auto const& segment : segments) {
            if (segment.empty())
                return geode::Err("native registration path contains an empty segment");
        }
        return geode::Ok(std::move(segments));
    }

    geode::Result<void> prepareNativeRegistration(
        geode::Mod* provider, std::string_view path, NativeRegistrationRequest& request
    ) {
        if (!provider) return geode::Err("native registration has no provider mod");

        request.providerId = std::string(provider->getID());
        if (request.providerId.empty())
            return geode::Err("native registration provider ID is empty");
        if (request.providerId.find('\0') != std::string::npos) {
            return geode::Err("native registration provider ID contains NUL");
        }

        auto parsed = parseNativePath(path);
        if (parsed.isErr()) return geode::Err(parsed.unwrapErr());
        request.segments = std::move(parsed.unwrap());
        request.qualifiedName = qualifiedName(request.providerId, request.segments);
        return geode::Ok();
    }

    geode::Result<luax::Runtime*> nativeRegistrationRuntime() {
        if (!luax::Runtime::isMainThread()) {
            return geode::Err("luau api must be called on the main thread");
        }
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }

        auto* runtime = luax::Runtime::getIfInitialized();
        if (!runtime || !runtime->ready()) return geode::Err("luau runtime not ready");
        return geode::Ok(runtime);
    }

    void pushNativeValue(lua_State* L, native_detail::NativeValue const& value) {
        switch (value.kind) {
            case native_detail::NativeValueKind::Nil: lua_pushnil(L); break;
            case native_detail::NativeValueKind::Boolean:
                lua_pushboolean(L, value.booleanValue);
                break;
            case native_detail::NativeValueKind::Integer:
                if (value.integerValue >= native_detail::kLuauMinSafeInteger &&
                    value.integerValue <= native_detail::kLuauMaxSafeInteger)
                    lua_pushnumber(L, static_cast<double>(value.integerValue));
                else lua_pushinteger64(L, value.integerValue);
                break;
            case native_detail::NativeValueKind::Number:
                lua_pushnumber(L, value.numberValue);
                break;
            case native_detail::NativeValueKind::String:
                lua_pushlstring(
                    L,
                    value.stringData ? value.stringData : "",
                    static_cast<std::size_t>(value.stringSize)
                );
                break;
            case native_detail::NativeValueKind::Unsupported:
                luaL_error(L, "unsupported native registered value");
                break;
        }
    }

    void pushNativeFunction(lua_State* L, NativeRegistrationRequest const& request) {
        std::size_t const nameOffset = sizeof(NativeClosureHeader) + request.functionBytes.size();
        std::size_t const totalSize = nameOffset + request.qualifiedName.size() + 1;
        auto* data = static_cast<std::byte*>(lua_newuserdata(L, totalSize));

        NativeClosureHeader header{
            request.invoker,
            request.functionBytes.size(),
            nameOffset,
        };
        std::memcpy(data, &header, sizeof(header));
        std::memcpy(data + sizeof(header), request.functionBytes.data(), request.functionBytes.size());
        std::memcpy(data + nameOffset, request.qualifiedName.c_str(), request.qualifiedName.size() + 1);

        char const* debugName = reinterpret_cast<char const*>(data + nameOffset);
        lua_pushcclosure(L, &nativeFunctionTrampoline, debugName, 1);
    }

    void pushNativeLeaf(lua_State* L, NativeRegistrationRequest const& request) {
        if (request.kind == NativeLeafKind::Function) pushNativeFunction(L, request);
        else pushNativeValue(L, request.value);
    }

    void pushDetachedNativeBranch(
        lua_State* L, NativeRegistrationRequest const& request, std::size_t nextSegment
    ) {
        lua_newtable(L);
        int const root = lua_absindex(L, -1);
        int current = root;

        for (std::size_t index = nextSegment; index + 1 < request.segments.size(); ++index) {
            lua_newtable(L);
            int const child = lua_absindex(L, -1);
            lua_pushvalue(L, child);
            lua_rawsetfield(L, current, request.segments[index].c_str());
            current = child;
        }

        pushNativeLeaf(L, request);
        lua_rawsetfield(L, current, request.segments.back().c_str());
        lua_settop(L, root);
    }

    int nativeRegistrationEntry(lua_State* L) {
        auto* request = static_cast<NativeRegistrationRequest*>(lua_touserdata(L, 1));
        if (!request) {
            luaL_error(L, "native registration request is missing");
            return 0;
        }

        lua_pushvalue(L, LUA_GLOBALSINDEX);
        int const globals = lua_absindex(L, -1);
        lua_rawgetfield(L, globals, request->providerId.c_str());

        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            pushDetachedNativeBranch(L, *request, 0);
            lua_rawsetfield(L, globals, request->providerId.c_str());
            return 0;
        }
        if (!lua_istable(L, -1)) {
            request->error = "native registration namespace is not a table: " + request->providerId;
            return 0;
        }

        lua_remove(L, globals);
        int current = lua_absindex(L, -1);
        for (std::size_t index = 0; index + 1 < request->segments.size(); ++index) {
            lua_rawgetfield(L, current, request->segments[index].c_str());
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                pushDetachedNativeBranch(L, *request, index + 1);
                lua_rawsetfield(L, current, request->segments[index].c_str());
                return 0;
            }
            if (!lua_istable(L, -1)) {
                request->error =
                    "native registration intermediate is not a table: " + request->segments[index];
                return 0;
            }
            lua_remove(L, current);
            current = lua_absindex(L, -1);
        }

        lua_rawgetfield(L, current, request->segments.back().c_str());
        if (!lua_isnil(L, -1)) {
            request->error = "native registration target already exists: " + request->qualifiedName;
            return 0;
        }
        lua_pop(L, 1);

        pushNativeLeaf(L, *request);
        lua_rawsetfield(L, current, request->segments.back().c_str());
        return 0;
    }

    geode::Result<void> runNativeRegistration(luax::Runtime& runtime, NativeRegistrationRequest& request) {
        auto* L = runtime.state();
        if (!L) return geode::Err("luau runtime not ready");

        luax::LuaStackGuard stack(L);
        int status = lua_cpcall(L, &nativeRegistrationEntry, &request);
        if (status != 0) {
            size_t size = 0;
            char const* data = lua_tolstring(L, -1, &size);
            std::string error = data ? std::string(data, size) : "unknown Luau error";
            return geode::Err("native registration failed for " + request.qualifiedName + ": " + error);
        }
        if (!request.error.empty()) return geode::Err(request.error);
        return geode::Ok();
    }

    struct PreparedRun {
        std::filesystem::path root;
        std::string source;
        std::string chunk;
    };

    geode::Result<void> requireMainThread() {
        if (!luax::Runtime::isMainThread()) {
            return geode::Err("luau api must be called on the main thread");
        }
        return geode::Ok();
    }

    geode::Result<void> requireSyncRunReady() {
        auto threadResult = requireMainThread();
        if (threadResult.isErr()) {
            return geode::Err(threadResult.unwrapErr());
        }
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }
        return geode::Ok();
    }

    geode::Result<void> requireAsyncRunReady() {
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }
        return geode::Ok();
    }

    geode::Result<std::string> prepareChunkName(std::string_view chunkName) {
        auto chunkResult = luax::normalizeVirtualPath(chunkName);
        if (chunkResult.isErr()) {
            return geode::Err(chunkResult.unwrapErr());
        }
        return geode::Ok("@" + luax::normalizedPathString(chunkResult.unwrap()));
    }

    geode::Result<void> executeScriptOnMain(
        std::filesystem::path const& root, std::string source, std::string chunk, int deadlineMs
    ) {
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }

        auto* runtime = luax::Runtime::getOrCreate();
        if (!runtime) {
            return geode::Err("luau runtime shutting down");
        }
        if (!runtime->ready()) {
            auto const& err = runtime->lastError();
            return geode::Err(!err.empty() ? err : "luau runtime not ready");
        }

        luax::Runtime::ResourcesRootScope rootScope(*runtime, root);

        auto result = runtime->runScript(source, chunk, deadlineMs);
        if (result.isErr()) {
            return geode::Err(result.unwrapErr());
        }

        return geode::Ok();
    }

    geode::Result<PreparedRun> prepareRunFile(
        std::filesystem::path const& resourcesRoot, std::filesystem::path const& relativePath
    ) {
        auto rootResult = luax::canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return geode::Err(rootResult.unwrapErr());
        }

        auto flatPathResult = luax::validateResourcePath(relativePath);
        if (flatPathResult.isErr()) {
            return geode::Err("relative path must be a flat .luau resource name");
        }
        auto flatPath = flatPathResult.unwrap();

        auto root = rootResult.unwrap();
        auto pathResult = luax::resolveScriptFileInsideRoot(root, root / flatPath);
        if (pathResult.isErr()) {
            return geode::Err(pathResult.unwrapErr());
        }
        auto path = pathResult.unwrap();

        std::error_code sizeEc;
        auto fileSize = std::filesystem::file_size(path, sizeEc);
        if (!sizeEc && fileSize > luax::kMaxScriptBytes) {
            return geode::Err("script file exceeds maximum size");
        }

        auto sourceResult = luax::readScriptFile(path);
        if (sourceResult.isErr()) {
            return geode::Err(sourceResult.unwrapErr());
        }

        std::error_code ec;
        auto rel = std::filesystem::relative(path, root, ec);
        auto chunkPath = ec ? path.filename() : rel;
        auto chunkResult = prepareChunkName(luax::normalizedPathString(chunkPath));
        if (chunkResult.isErr()) {
            return geode::Err(chunkResult.unwrapErr());
        }

        return geode::Ok(
            PreparedRun{std::move(root), std::move(sourceResult.unwrap()), chunkResult.unwrap()}
        );
    }

    geode::Result<PreparedRun> prepareRunScript(
        std::filesystem::path const& resourcesRoot, std::string_view chunkName,
        std::string_view sourceBytes
    ) {
        auto rootResult = luax::canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return geode::Err(rootResult.unwrapErr());
        }

        auto chunkResult = prepareChunkName(chunkName);
        if (chunkResult.isErr()) {
            return geode::Err(chunkResult.unwrapErr());
        }

        if (sourceBytes.size() > luax::kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }

        return geode::Ok(
            PreparedRun{rootResult.unwrap(), std::string(sourceBytes), chunkResult.unwrap()}
        );
    }

#if !defined(LUAUAPI_HOST_TESTS)
    arc::Future<geode::Result<void>> executePreparedRunAsync(PreparedRun run, int deadlineMs) {
        auto result = co_await geode::async::waitForMainThread<geode::Result<void>>(
            [root = std::move(run.root),
             chunk = std::move(run.chunk),
             source = std::move(run.source),
             deadlineMs]() mutable {
                return executeScriptOnMain(root, std::move(source), chunk, deadlineMs);
            }
        );
        co_return imes::luauapi::resolveAsyncMainThreadResult(result);
    }
#endif
} // namespace

namespace imes::luauapi {
    namespace detail {
        geode::Result<void> registerNativeFunction(
            geode::Mod* provider, char const* pathData, std::uint64_t pathSize,
            NativeInvoker invoker, void const* functionBytes, std::uint64_t functionSize
        ) {
            auto runtimeResult = nativeRegistrationRuntime();
            if (runtimeResult.isErr()) return geode::Err(runtimeResult.unwrapErr());
            if ((!pathData && pathSize != 0) || pathSize > (std::numeric_limits<std::size_t>::max)()) {
                return geode::Err("native registration path is invalid");
            }
            if (!invoker || !functionBytes || functionSize == 0) {
                return geode::Err("native function descriptor is invalid");
            }
            if (functionSize > kMaxNativeFunctionBytes) {
                return geode::Err("native function pointer representation is too large");
            }

            NativeRegistrationRequest request;
            request.kind = NativeLeafKind::Function;
            request.invoker = invoker;
            request.functionBytes.resize(static_cast<std::size_t>(functionSize));
            std::memcpy(
                request.functionBytes.data(), functionBytes, static_cast<std::size_t>(functionSize)
            );

            std::string_view path = pathData ?
                std::string_view(pathData, static_cast<std::size_t>(pathSize)) :
                std::string_view{};
            auto prepared = prepareNativeRegistration(provider, path, request);
            if (prepared.isErr()) return geode::Err(prepared.unwrapErr());
            if (request.qualifiedName.size() > std::numeric_limits<std::size_t>::max() -
                    sizeof(NativeClosureHeader) - request.functionBytes.size() - 1) {
                return geode::Err("native registration target is too large");
            }
            return runNativeRegistration(*runtimeResult.unwrap(), request);
        }

        geode::Result<void> registerNativeValue(
            geode::Mod* provider, char const* pathData, std::uint64_t pathSize,
            NativeValue const* valueData
        ) {
            auto runtimeResult = nativeRegistrationRuntime();
            if (runtimeResult.isErr()) return geode::Err(runtimeResult.unwrapErr());
            if ((!pathData && pathSize != 0) || pathSize > (std::numeric_limits<std::size_t>::max)()) {
                return geode::Err("native registration path is invalid");
            }
            if (!valueData) return geode::Err("registered value descriptor is invalid");
            auto const value = *valueData;

            NativeRegistrationRequest request;
            request.kind = NativeLeafKind::Value;
            request.value = value;

            switch (value.kind) {
                case NativeValueKind::Nil: return geode::Err("registered value cannot be nil");
                case NativeValueKind::Number:
                    if (!std::isfinite(value.numberValue)) {
                        return geode::Err("registered number must be finite");
                    }
                    break;
                case NativeValueKind::String:
                    if (!value.stringData && value.stringSize != 0) {
                        return geode::Err("registered string data is null");
                    }
                    if (value.stringSize > (std::numeric_limits<std::size_t>::max)()) {
                        return geode::Err("registered string is too large");
                    }
                    request.valueString.assign(
                        value.stringData ? value.stringData : "",
                        static_cast<std::size_t>(value.stringSize)
                    );
                    request.value.stringData = request.valueString.data();
                    request.value.stringSize = static_cast<std::uint64_t>(request.valueString.size());
                    break;
                case NativeValueKind::Boolean:
                case NativeValueKind::Integer: break;
                case NativeValueKind::Unsupported:
                    return geode::Err("registered value kind is unsupported");
            }

            std::string_view path = pathData ?
                std::string_view(pathData, static_cast<std::size_t>(pathSize)) :
                std::string_view{};
            auto prepared = prepareNativeRegistration(provider, path, request);
            if (prepared.isErr()) return geode::Err(prepared.unwrapErr());
            return runNativeRegistration(*runtimeResult.unwrap(), request);
        }
    } // namespace detail

    geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot, std::filesystem::path const& relativePath,
        int deadlineMs
    ) {
        auto readyResult = requireSyncRunReady();
        if (readyResult.isErr()) {
            return geode::Err(readyResult.unwrapErr());
        }

        auto prepared = prepareRunFile(resourcesRoot, relativePath);
        if (prepared.isErr()) {
            return geode::Err(prepared.unwrapErr());
        }

        auto [root, source, chunk] = std::move(prepared.unwrap());
        return executeScriptOnMain(std::move(root), std::move(source), std::move(chunk), deadlineMs);
    }

    geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot, std::string_view source,
        std::string_view chunkName, int deadlineMs
    ) {
        auto readyResult = requireSyncRunReady();
        if (readyResult.isErr()) {
            return geode::Err(readyResult.unwrapErr());
        }

        auto prepared = prepareRunScript(resourcesRoot, chunkName, source);
        if (prepared.isErr()) {
            return geode::Err(prepared.unwrapErr());
        }

        auto [root, preparedSource, chunk] = std::move(prepared.unwrap());
        return executeScriptOnMain(
            std::move(root), std::move(preparedSource), std::move(chunk), deadlineMs
        );
    }

    geode::Result<void> resolveAsyncMainThreadResult(std::optional<geode::Result<void>> const& result) {
        if (!result) {
            if (luax::Runtime::isShuttingDown()) {
                return geode::Err("luau runtime shutting down");
            }
            return geode::Err("luau main-thread execution cancelled");
        }
        return *result;
    }

#if !defined(LUAUAPI_HOST_TESTS)
    arc::Future<geode::Result<void>> runFileAsync(
        std::filesystem::path resourcesRoot, std::filesystem::path relativePath, int deadlineMs
    ) {
        auto readyResult = requireAsyncRunReady();
        if (readyResult.isErr()) {
            co_return geode::Err(readyResult.unwrapErr());
        }

        auto prepared = prepareRunFile(resourcesRoot, relativePath);
        if (prepared.isErr()) {
            co_return geode::Err(prepared.unwrapErr());
        }

        co_return co_await executePreparedRunAsync(std::move(prepared.unwrap()), deadlineMs);
    }

    arc::Future<geode::Result<void>> runScriptAsync(
        std::filesystem::path resourcesRoot, std::string source, std::string chunkName, int deadlineMs
    ) {
        auto readyResult = requireAsyncRunReady();
        if (readyResult.isErr()) {
            co_return geode::Err(readyResult.unwrapErr());
        }

        auto prepared = prepareRunScript(resourcesRoot, chunkName, source);
        if (prepared.isErr()) {
            co_return geode::Err(prepared.unwrapErr());
        }

        co_return co_await executePreparedRunAsync(std::move(prepared.unwrap()), deadlineMs);
    }
#endif

    bool isReady() {
        if (luax::Runtime::isShuttingDown()) return false;
        if (!luax::Runtime::isMainThread()) return false;
        return luax::Runtime::isInitialized();
    }

    RuntimeStatus status() {
        if (luax::Runtime::isShuttingDown()) return RuntimeStatus::NotReady;
        if (!luax::Runtime::isMainThread()) return RuntimeStatus::NotReady;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->status() : RuntimeStatus::NotReady;
    }

    std::string lastError() {
        if (luax::Runtime::isShuttingDown()) return {};
        if (!luax::Runtime::isMainThread()) return {};
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->lastError() : std::string{};
    }

    std::size_t memoryUsage() {
        if (luax::Runtime::isShuttingDown()) return 0;
        if (!luax::Runtime::isMainThread()) return 0;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->memoryUsage() : 0;
    }

    std::size_t memoryLimit() {
        if (luax::Runtime::isShuttingDown()) return 0;
        if (!luax::Runtime::isMainThread()) return 0;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->memoryLimit() : 0;
    }

    bool codegenEnabled() {
        if (luax::Runtime::isShuttingDown()) return false;
        if (!luax::Runtime::isMainThread()) return false;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime && runtime->codegenEnabled();
    }
} // namespace imes::luauapi
