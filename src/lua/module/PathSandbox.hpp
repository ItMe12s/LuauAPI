#pragma once

#include "lua/Config.hpp"
#include "PathRules.hpp"

#include <Geode/Result.hpp>
#include <Geode/utils/string.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace luax {
    inline std::string normalizedPathString(std::filesystem::path const& path) {
        return path.generic_string();
    }

    inline std::string filesystemPathString(std::filesystem::path const& path) {
#if defined(LUAUAPI_HOST_TESTS)
        auto text = path.u8string();
        return std::string(reinterpret_cast<char const*>(text.data()), text.size());
#else
        return geode::utils::string::pathToString(path);
#endif
    }

    inline geode::Result<std::string> readScriptFile(std::filesystem::path const& path) {
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (!ec && size > kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }

        std::ifstream in(path, std::ios::binary);
        if (!in.good()) {
            return geode::Err("script cannot be read: " + filesystemPathString(path));
        }

        std::string contents;
        if (!ec) {
            contents.reserve(static_cast<std::size_t>(size));
        }

        std::vector<char> buffer(64 * 1024);
        while (in) {
            in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            auto read = in.gcount();
            if (read <= 0) break;
            if (contents.size() + static_cast<std::size_t>(read) > kMaxScriptBytes) {
                return geode::Err("script exceeds maximum size");
            }
            contents.append(buffer.data(), static_cast<std::size_t>(read));
        }
        if (in.bad()) {
            return geode::Err("script cannot be read: " + filesystemPathString(path));
        }
        return geode::Ok(std::move(contents));
    }

    inline geode::Result<std::filesystem::path> resolveScriptFileInsideRoot(
        std::filesystem::path const& root,
        std::filesystem::path const& candidate
    ) {
        std::error_code ec;
        auto path = std::filesystem::weakly_canonical(candidate, ec);
        if (ec) {
            return geode::Err("script path cannot be resolved: " + ec.message());
        }

        if (!pathInsideRootValue(path, root)) {
            return geode::Err("script path escapes resources root");
        }

        if (!std::filesystem::is_regular_file(path, ec)) {
            return geode::Err("script file not found: " + filesystemPathString(path));
        }

        return geode::Ok(path);
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
            return geode::Err("resources root is not a directory: " + filesystemPathString(root));
        }

        return geode::Ok(root);
    }
}
