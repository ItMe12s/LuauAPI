#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#if defined(LUAUAPI_HOST_TESTS)
    #define LUAUAPI_DLL
#elif defined(GEODE_IS_WINDOWS)
    #ifdef LUAUAPI_EXPORTING
        #define LUAUAPI_DLL __declspec(dllexport)
    #else
        #define LUAUAPI_DLL __declspec(dllimport)
    #endif
#else
    #ifdef LUAUAPI_EXPORTING
        #define LUAUAPI_DLL __attribute__((visibility("default")))
    #else
        #define LUAUAPI_DLL
    #endif
#endif

// I hope people actually read this.
namespace imes::luauapi {
    inline constexpr int kDefaultScriptDeadlineMs = 250;

    enum class RuntimeStatus {
        NotReady,
        Ready,
        InitFailed,
        Panicked,
    };

    // Shared singleton VM for all dependent mods. Call API on main thread.
    // deadlineMs <= 0 disables Luau CPU budget for this execution.
    LUAUAPI_DLL geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot,
        std::filesystem::path const& relativePath,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    // chunkName is a flat Luau resource name under resourcesRoot. No subdirs allowed.
    // Panic is terminal for this process. OOM denies allocation from shared pool.
    LUAUAPI_DLL geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot,
        std::string_view source,
        std::string_view chunkName,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    LUAUAPI_DLL arc::Future<geode::Result<void>> runFileAsync(
        std::filesystem::path resourcesRoot,
        std::filesystem::path relativePath,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    LUAUAPI_DLL arc::Future<geode::Result<void>> runScriptAsync(
        std::filesystem::path resourcesRoot,
        std::string source,
        std::string chunkName,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    // Compatibility check for RuntimeStatus::Ready.
    LUAUAPI_DLL bool isReady();

    // Does not create the runtime. Returns NotReady before init or off main thread.
    LUAUAPI_DLL RuntimeStatus status();

    // Synchronous error side channel. Read before next LuauAPI call.
    // Value is not stable across reentrant LuauAPI calls.
    // Does not create the runtime.
    LUAUAPI_DLL std::string_view lastError();

    // Main-thread only query APIs. Off-thread calls return default values.
    LUAUAPI_DLL std::size_t memoryUsage();
    LUAUAPI_DLL std::size_t memoryLimit();

    LUAUAPI_DLL bool codegenEnabled();
}
