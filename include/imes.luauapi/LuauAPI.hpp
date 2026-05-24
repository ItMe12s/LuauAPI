#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#ifdef GEODE_IS_WINDOWS
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

namespace imes::luauapi {
    enum class RuntimeStatus {
        NotReady,
        Ready,
        InitFailed,
        Panicked,
    };

    // Shared VM per mod. Call API on main thread. deadlineMs <= 0 disables CPU budget.
    LUAUAPI_DLL geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot,
        std::filesystem::path const& relativePath,
        int deadlineMs = 250
    );

    // chunkName is a flat Luau resource name under resourcesRoot. No subdirs allowed.
    // Panic is fatal, OOM denies allocation.
    LUAUAPI_DLL geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot,
        std::string_view source,
        std::string_view chunkName,
        int deadlineMs = 250
    );

    LUAUAPI_DLL geode::Task<geode::Result<void>> runFileAsync(
        std::filesystem::path const& resourcesRoot,
        std::filesystem::path const& relativePath,
        int deadlineMs = 250
    );

    LUAUAPI_DLL geode::Task<geode::Result<void>> runScriptAsync(
        std::filesystem::path const& resourcesRoot,
        std::string source,
        std::string_view chunkName,
        int deadlineMs = 250
    );

    // Compatibility check for RuntimeStatus::Ready.
    LUAUAPI_DLL bool isReady();

    // Does not create the runtime. Returns NotReady before init or off main thread.
    LUAUAPI_DLL RuntimeStatus status();

    // Synchronous error side channel. Read before the next LuauAPI call.
    // Does not create the runtime.
    LUAUAPI_DLL std::string_view lastError();

    // Main-thread only query APIs. Off-thread calls return default values.
    LUAUAPI_DLL std::size_t memoryUsage();
    LUAUAPI_DLL std::size_t memoryLimit();
    LUAUAPI_DLL bool codegenEnabled();
}
