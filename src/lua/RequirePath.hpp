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
        return isFlatResourcePathValue(std::filesystem::path(name));
    }

    inline std::filesystem::path requireModulePath(std::filesystem::path current) {
        current += ".luau";
        return current;
    }
}
