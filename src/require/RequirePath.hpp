#pragma once

#include "require/PathRules.hpp"

#include <Geode/utils/string.hpp>
#include <array>
#include <filesystem>
#include <string_view>

namespace luax {
    inline bool canRequireFromChunk(std::string_view requirerChunkname) {
        return geode::utils::string::startsWith(requirerChunkname, "@");
    }

    inline bool isRequireChildNameAllowed(std::string_view name) {
        if (name.empty() || name == "..") {
            return false;
        }
        if (geode::utils::string::containsAny(name, std::array<std::string, 2>{"/", "\\"})) {
            return false;
        }
        return isValidResourcePathValue(std::filesystem::path(name), false);
    }

    inline std::filesystem::path requireModulePath(std::filesystem::path current) {
        if (!hasLuauExtensionValue(current)) {
            current += ".luau";
        }
        return current;
    }
} // namespace luax
