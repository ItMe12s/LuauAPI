#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace luax {
    std::string bytecodeCacheKey(std::filesystem::path const& path, std::string const& contents);
    std::string loadstringBytecodeKey(std::string_view chunkName, std::string_view source);
    std::string runScriptBytecodeKey(
        std::filesystem::path const& resourcesRoot, std::string_view chunkName, std::string_view source
    );
} // namespace luax
