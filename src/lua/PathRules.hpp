#pragma once

#include <Geode/utils/string.hpp>

#include <filesystem>
#include <string_view>
#include <system_error>

namespace luax {
    inline bool escapedRelativePathText(std::string_view text) {
        return text == ".."
            || geode::utils::string::startsWith(text, "../")
            || geode::utils::string::startsWith(text, "..\\");
    }

    inline bool escapedRelativePathValue(std::filesystem::path const& rel) {
        return rel.empty() || escapedRelativePathText(geode::utils::string::pathToString(rel));
    }

    inline bool pathInsideRootValue(std::filesystem::path const& path, std::filesystem::path const& root) {
        std::error_code ec;
        auto rel = std::filesystem::relative(path, root, ec);
        return !ec && !escapedRelativePathValue(rel);
    }

    inline bool isFlatResourcePathValue(std::filesystem::path const& path) {
        auto normalized = path.lexically_normal();
        return normalized == normalized.filename()
            && normalized != "."
            && normalized != ".."
            && !normalized.empty();
    }

    inline bool hasLuauExtensionValue(std::filesystem::path const& path) {
        return path.extension() == ".luau";
    }

    inline bool hasUnsupportedExtensionValue(std::filesystem::path const& path) {
        auto ext = path.extension();
        return !ext.empty() && ext != ".luau";
    }
}
