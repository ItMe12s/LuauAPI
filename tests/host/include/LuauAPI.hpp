#pragma once

#include <Geode/Geode.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#if defined(GEODE_IS_WINDOWS)
    #define LUAUAPI_DLL
#else
    #define LUAUAPI_DLL
#endif

#include <RuntimeTypes.hpp>

namespace imes::luauapi {
    geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot,
        std::filesystem::path const& relativePath,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot,
        std::string_view source,
        std::string_view chunkName,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    bool isReady();
    RuntimeStatus status();
    std::string lastError();
    std::size_t memoryUsage();
    std::size_t memoryLimit();
    bool codegenEnabled();
}
