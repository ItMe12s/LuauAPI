#pragma once

#include <Geode/Geode.hpp>

#include <cstddef>
#include <filesystem>
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
    LUAUAPI_DLL geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot,
        std::filesystem::path const& relativePath,
        int deadlineMs = 250
    );

    // chunkName is a flat Luau resource name under resourcesRoot.
    // It may be "Main", "Main.luau", or "@Main.luau".
    // Subdirectories are rejected because Geode doesn't support them anyway.
    LUAUAPI_DLL geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot,
        std::string_view source,
        std::string_view chunkName,
        int deadlineMs = 250
    );

    LUAUAPI_DLL bool isReady();
    LUAUAPI_DLL std::size_t memoryUsage();
    LUAUAPI_DLL std::size_t memoryLimit();
    LUAUAPI_DLL bool codegenEnabled();
}
