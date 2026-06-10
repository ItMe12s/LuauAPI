#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

namespace luax {
    inline bool escapedRelativePathText(std::string_view text) {
        return text == ".." || text.starts_with("../") || text.starts_with("..\\");
    }

    inline bool escapedRelativePathValue(std::filesystem::path const& rel) {
        return rel.empty() || escapedRelativePathText(rel.generic_string());
    }

    inline bool pathInsideRootValue(std::filesystem::path const& path, std::filesystem::path const& root) {
        std::error_code ec;
        auto rel = std::filesystem::relative(path, root, ec);
        return !ec && !escapedRelativePathValue(rel);
    }

    inline bool isFlatResourcePathValue(std::filesystem::path const& path) {
        auto normalized = path.lexically_normal();
        return normalized == normalized.filename() && normalized != "." && normalized != ".." &&
            !normalized.empty();
    }

    inline bool hasLuauExtensionValue(std::filesystem::path const& path) {
        return path.extension() == ".luau";
    }

    inline bool hasUnsupportedExtensionValue(std::filesystem::path const& path) {
        auto ext = path.extension();
        return !ext.empty() && ext != ".luau";
    }

    inline bool isValidResourcePathValue(std::filesystem::path const& path, bool addLuauExtension = true) {
        if (path.empty() || path.is_absolute()) {
            return false;
        }

        auto normalized = path.lexically_normal();
        if (!isFlatResourcePathValue(normalized) || hasUnsupportedExtensionValue(normalized)) {
            return false;
        }

        if (!addLuauExtension) {
            return true;
        }

        auto withExtension = normalized;
        if (!hasLuauExtensionValue(withExtension)) {
            withExtension += ".luau";
        }
        return isFlatResourcePathValue(withExtension);
    }
} // namespace luax
