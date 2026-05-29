#pragma once

#include "PathRules.hpp"

#include <filesystem>
#include <string_view>

namespace luax {
    inline bool canRequireFromChunk(std::string_view requirerChunkname) {
        return !requirerChunkname.empty() && requirerChunkname.front() == '@';
    }

    inline bool isRequireChildNameAllowed(std::string_view name) {
        if (name.empty() || name == "..") {
            return false;
        }
        if (name.find('/') != std::string_view::npos || name.find('\\') != std::string_view::npos) {
            return false;
        }
        auto path = std::filesystem::path(name);
        return isFlatResourcePathValue(path) && !hasUnsupportedExtensionValue(path);
    }

    inline std::filesystem::path requireModulePath(std::filesystem::path current) {
        if (!hasLuauExtensionValue(current)) {
            current += ".luau";
        }
        return current;
    }
}
