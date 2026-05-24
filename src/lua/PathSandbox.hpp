#pragma once

#include "Config.hpp"

#include <Geode/utils/file.hpp>
#include <Geode/utils/string.hpp>

#include <filesystem>
#include <string>
#include <string_view>

namespace luax {
    inline std::string normalizedPathString(std::filesystem::path const& path) {
        return geode::utils::string::pathToString(path);
    }

    inline geode::Result<std::string> readScriptFile(std::filesystem::path const& path) {
        auto result = geode::utils::file::readString(path);
        if (result.isErr()) {
            return geode::Err(result.unwrapErr());
        }
        auto contents = std::move(result.unwrap());
        if (contents.size() > kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }
        return geode::Ok(std::move(contents));
    }

    inline bool escapesRoot(std::filesystem::path const& rel) {
        auto s = normalizedPathString(rel);
        return rel.empty() || s == ".." || s.rfind("../", 0) == 0;
    }

    inline bool pathInsideRoot(std::filesystem::path const& path, std::filesystem::path const& root) {
        std::error_code ec;
        auto rel = std::filesystem::relative(path, root, ec);
        return !ec && !escapesRoot(rel);
    }

    inline bool hasLuauExtension(std::filesystem::path const& path) {
        return path.extension() == ".luau";
    }

    inline bool hasUnsupportedExtension(std::filesystem::path const& path) {
        auto ext = path.extension();
        return !ext.empty() && ext != ".luau";
    }

    inline bool isFlatResourcePath(std::filesystem::path const& path) {
        auto normalized = path.lexically_normal();
        return normalized == normalized.filename()
            && normalized != "."
            && normalized != ".."
            && !normalized.empty();
    }

    inline geode::Result<std::filesystem::path> normalizeVirtualPath(std::string_view rawChunkName) {
        if (rawChunkName.empty()) {
            return geode::Err("chunk name is empty");
        }

        std::string text(rawChunkName);
        if (!text.empty() && text.front() == '@') {
            text.erase(text.begin());
        }

        std::filesystem::path path(text);
        if (path.empty()) {
            return geode::Err("chunk name is empty");
        }

        if (path.is_absolute()) {
            return geode::Err("chunk name must not be absolute");
        }

        path = path.lexically_normal();
        if (!isFlatResourcePath(path)) {
            return geode::Err("chunk name must be a flat resource name");
        }

        if (hasUnsupportedExtension(path)) {
            return geode::Err("chunk name extension must be .luau");
        }

        if (!hasLuauExtension(path)) {
            path += ".luau";
        }

        if (!isFlatResourcePath(path)) {
            return geode::Err("chunk name must be a flat resource name");
        }

        return geode::Ok(path);
    }

    inline geode::Result<std::filesystem::path> canonicalRoot(std::filesystem::path const& resourcesRoot) {
        if (resourcesRoot.empty()) {
            return geode::Err("resources root is empty");
        }

        std::error_code ec;
        auto root = std::filesystem::weakly_canonical(resourcesRoot, ec);
        if (ec) {
            return geode::Err("resources root cannot be resolved: " + ec.message());
        }

        if (!std::filesystem::is_directory(root, ec)) {
            return geode::Err("resources root is not a directory: " + normalizedPathString(root));
        }

        return geode::Ok(root);
    }
}
