#pragma once

#include "core/Config.hpp"

#include <Geode/Result.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/string.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace luax {
    [[nodiscard]] inline geode::Result<std::filesystem::path> validateResourcePath(
        std::filesystem::path path, bool addLuauExtension = true
    );

    inline bool escapedRelativePathText(std::string_view text) {
        return text == ".." || geode::utils::string::startsWith(text, "../") ||
            geode::utils::string::startsWith(text, "..\\");
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
        return validateResourcePath(path, addLuauExtension).isOk();
    }

    inline bool canRequireFromChunk(std::string_view requirerChunkname) {
        return geode::utils::string::startsWith(requirerChunkname, "@");
    }

    inline bool isRequireChildNameAllowed(std::string_view name) {
        if (name.empty() || name == "..") {
            return false;
        }
        if (name.find_first_of("/\\") != std::string_view::npos) {
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

    // If anyone misuses these I swear to god.
    // POSIX-style path text for virtual chunk names and sandbox-relative checks.
    inline std::string normalizedPathString(std::filesystem::path const& path) {
        return path.generic_string();
    }

    inline std::string filesystemPathString(std::filesystem::path const& path) {
        return geode::utils::string::pathToString(path);
    }

    [[nodiscard]] inline geode::Result<std::string> readScriptFile(std::filesystem::path const& path) {
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (!ec && size > kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }

        auto contents = geode::utils::file::readString(path);
        if (contents.isErr()) {
            return geode::Err("script cannot be read: " + filesystemPathString(path));
        }
        auto data = std::move(contents.unwrap());
        if (data.size() > kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }
        return geode::Ok(std::move(data));
    }

    [[nodiscard]] inline geode::Result<std::filesystem::path> resolveScriptFileInsideRoot(
        std::filesystem::path const& root, std::filesystem::path const& candidate
    ) {
        if (root.empty()) {
            return geode::Err("resources root is not configured");
        }

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

    [[nodiscard]] inline geode::Result<std::filesystem::path> validateResourcePath(
        std::filesystem::path path, bool addLuauExtension
    ) {
        if (path.empty()) {
            return geode::Err("resource path is empty");
        }

        if (path.is_absolute()) {
            return geode::Err("resource path must not be absolute");
        }

        path = path.lexically_normal();
        if (!isFlatResourcePathValue(path)) {
            return geode::Err("resource path must be a flat resource name");
        }

        if (hasUnsupportedExtensionValue(path)) {
            return geode::Err("resource path extension must be .luau");
        }

        if (addLuauExtension && !hasLuauExtensionValue(path)) {
            path += ".luau";
        }

        if (!isFlatResourcePathValue(path)) {
            return geode::Err("resource path must be a flat resource name");
        }

        return geode::Ok(std::move(path));
    }

    [[nodiscard]] inline geode::Result<std::filesystem::path> normalizeVirtualPath(
        std::string_view rawChunkName
    ) {
        if (rawChunkName.empty()) {
            return geode::Err("chunk name is empty");
        }

        std::string text(rawChunkName);
        if (geode::utils::string::startsWith(text, "@")) {
            text.erase(text.begin());
        }

        if (text.empty()) {
            return geode::Err("chunk name is empty");
        }

        auto validated = validateResourcePath(std::filesystem::path(text));
        if (validated.isErr()) {
            auto const& message = validated.unwrapErr();
            if (message == "resource path is empty") {
                return geode::Err("chunk name is empty");
            }
            if (message == "resource path must not be absolute") {
                return geode::Err("chunk name must not be absolute");
            }
            if (message == "resource path must be a flat resource name") {
                return geode::Err("chunk name must be a flat resource name");
            }
            if (message == "resource path extension must be .luau") {
                return geode::Err("chunk name extension must be .luau");
            }
            return geode::Err(message);
        }

        return validated;
    }

    [[nodiscard]] inline geode::Result<std::filesystem::path> canonicalRoot(
        std::filesystem::path const& resourcesRoot
    ) {
        static thread_local std::optional<
            std::pair<std::filesystem::path, geode::Result<std::filesystem::path>>>
            cache;
        if (cache && cache->first == resourcesRoot) {
            return cache->second;
        }

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

        cache.emplace(resourcesRoot, geode::Ok(root));
        return geode::Ok(root);
    }

    [[nodiscard]] inline geode::Result<std::filesystem::path> resolveInsideRoot(
        std::filesystem::path const& root, std::string_view relative
    ) {
        if (relative.empty()) {
            return geode::Err("path is empty");
        }

        std::filesystem::path rel(relative);
        if (rel.is_absolute()) {
            return geode::Err("path must be relative");
        }

        auto canonRoot = canonicalRoot(root);
        if (canonRoot.isErr()) {
            return geode::Err(canonRoot.unwrapErr());
        }

        std::error_code ec;
        auto resolved = std::filesystem::weakly_canonical(canonRoot.unwrap() / rel, ec);
        if (ec) {
            return geode::Err("path cannot be resolved: " + ec.message());
        }

        if (!pathInsideRootValue(resolved, canonRoot.unwrap())) {
            return geode::Err("path escapes the root directory");
        }

        return geode::Ok(resolved);
    }
} // namespace luax
