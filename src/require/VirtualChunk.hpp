#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace geode {
    class Mod;
}

namespace luax {
    constexpr char kVirtualChunkModSeparator = ':';

    struct VirtualChunkParts {
        std::string_view modId;
        std::string_view scriptPath;
    };

    std::string_view stripVirtualChunkAt(std::string_view chunk);

    VirtualChunkParts parseVirtualChunk(std::string_view chunk);

    std::string_view virtualChunkScriptPath(std::string_view chunk);

    bool virtualChunkHasModPrefix(std::string_view chunk);

    std::string formatVirtualChunk(std::string_view modId, std::string_view scriptPath);

    std::optional<std::string> modIdForResourcesRoot(std::filesystem::path const& resourcesRoot);

    geode::Mod* modForResourcesRoot(std::filesystem::path const& resourcesRoot);

    std::string enrichCallbackContext(
        std::filesystem::path const& resourcesRoot, std::string_view context
    );
} // namespace luax
