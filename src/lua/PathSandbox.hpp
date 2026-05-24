#pragma once

#include "Config.hpp"
#include "PathRules.hpp"

#include <Geode/Result.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

namespace luax {
    inline std::string normalizedPathString(std::filesystem::path const& path) {
        return path.generic_string();
    }

    inline geode::Result<std::string> readScriptFile(std::filesystem::path const& path) {
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (!ec && size > kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }

        std::ifstream in(path, std::ios::binary);
        if (!in.good()) {
            return geode::Err("script cannot be read: " + normalizedPathString(path));
        }

        std::string contents(
            std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()
        );
        if (in.bad()) {
            return geode::Err("script cannot be read: " + normalizedPathString(path));
        }

        if (contents.size() > kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }
        return geode::Ok(std::move(contents));
    }

    inline bool escapesRoot(std::filesystem::path const& rel) {
        auto s = normalizedPathString(rel);
        return rel.empty() || escapedRelativePathText(s);
    }

    inline bool pathInsideRoot(std::filesystem::path const& path, std::filesystem::path const& root) {
        return pathInsideRootValue(path, root);
    }

    inline bool hasLuauExtension(std::filesystem::path const& path) {
        return hasLuauExtensionValue(path);
    }

    inline bool hasUnsupportedExtension(std::filesystem::path const& path) {
        return hasUnsupportedExtensionValue(path);
    }

    inline bool isFlatResourcePath(std::filesystem::path const& path) {
        return isFlatResourcePathValue(path);
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
