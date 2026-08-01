#pragma once

#include "Export.hpp"
#include "NativeRegistration.hpp"
#include "RuntimeTypes.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace imes::luauapi {
    LUAUAPI_DLL geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot, std::filesystem::path const& relativePath,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    LUAUAPI_DLL geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot, std::string_view source,
        std::string_view chunkName, int deadlineMs = kDefaultScriptDeadlineMs
    );

    LUAUAPI_DLL arc::Future<geode::Result<void>> runFileAsync(
        std::filesystem::path resourcesRoot, std::filesystem::path relativePath,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    LUAUAPI_DLL arc::Future<geode::Result<void>> runScriptAsync(
        std::filesystem::path resourcesRoot, std::string source, std::string chunkName,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    LUAUAPI_DLL bool isReady();

    LUAUAPI_DLL RuntimeStatus status();

    LUAUAPI_DLL std::string lastError();

    LUAUAPI_DLL std::size_t memoryUsage();
    LUAUAPI_DLL std::size_t memoryLimit();

    LUAUAPI_DLL bool codegenEnabled();
} // namespace imes::luauapi
