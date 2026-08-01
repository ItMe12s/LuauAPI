#pragma once

#include <Export.hpp>
#include <Geode/Geode.hpp>
#include <NativeRegistration.hpp>
#include <RuntimeTypes.hpp>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace imes::luauapi {
    geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot, std::filesystem::path const& relativePath,
        int deadlineMs = kDefaultScriptDeadlineMs
    );

    geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot, std::string_view source,
        std::string_view chunkName, int deadlineMs = kDefaultScriptDeadlineMs
    );

    bool isReady();
    RuntimeStatus status();
    std::string lastError();
    std::size_t memoryUsage();
    std::size_t memoryLimit();
    bool codegenEnabled();
} // namespace imes::luauapi
