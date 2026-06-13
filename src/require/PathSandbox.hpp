#pragma once

#include "core/Config.hpp"
#include "require/PathRules.hpp"

#if defined(LUAUAPI_HOST_TESTS)
    #include <Geode/utils/string.hpp>
    #include <fstream>
    #include <optional>
    #include <vector>
#else
    #include <Geode/Result.hpp>
    #include <Geode/utils/file.hpp>
    #include <Geode/utils/string.hpp>
#endif

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace luax {
#if defined(LUAUAPI_HOST_TESTS)
    template <class T>
    class HostResult {
    public:
        static HostResult ok(T value) {
            HostResult result;
            result.m_value = std::move(value);
            return result;
        }

        static HostResult err(std::string message) {
            HostResult result;
            result.m_error = std::move(message);
            return result;
        }

        bool isOk() const {
            return m_value.has_value();
        }

        bool isErr() const {
            return !isOk();
        }

        T& unwrap() {
            return *m_value;
        }

        T const& unwrap() const {
            return *m_value;
        }

        std::string const& unwrapErr() const {
            return m_error;
        }

    private:
        std::optional<T> m_value;
        std::string m_error;
    };

    template <class T>
    using ScriptResult = HostResult<T>;

    template <class T>
    inline ScriptResult<T> scriptOk(T value) {
        return ScriptResult<T>::ok(std::move(value));
    }

    template <class T>
    inline ScriptResult<T> scriptErr(std::string message) {
        return ScriptResult<T>::err(std::move(message));
    }
#else
    template <class T>
    using ScriptResult = geode::Result<T>;

    template <class T>
    inline ScriptResult<T> scriptOk(T value) {
        return geode::Ok(std::move(value));
    }

    template <class T>
    inline ScriptResult<T> scriptErr(std::string message) {
        return geode::Err(std::move(message));
    }
#endif

    // If anyone misuses theses I swear to god.
    // POSIX-style path text for virtual chunk names and sandbox-relative checks.
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

    inline ScriptResult<std::string> readScriptFile(std::filesystem::path const& path) {
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (!ec && size > kMaxScriptBytes) {
            return scriptErr<std::string>("script exceeds maximum size");
        }

#if defined(LUAUAPI_HOST_TESTS)
        std::ifstream in(path, std::ios::binary);
        if (!in.good()) {
            return scriptErr<std::string>("script cannot be read: " + filesystemPathString(path));
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
                return scriptErr<std::string>("script exceeds maximum size");
            }
            contents.append(buffer.data(), static_cast<std::size_t>(read));
        }
        if (in.bad()) {
            return scriptErr<std::string>("script cannot be read: " + filesystemPathString(path));
        }
        return scriptOk(std::move(contents));
#else
        auto contents = geode::utils::file::readString(path);
        if (contents.isErr()) {
            return scriptErr<std::string>("script cannot be read: " + filesystemPathString(path));
        }
        auto data = std::move(contents.unwrap());
        if (data.size() > kMaxScriptBytes) {
            return scriptErr<std::string>("script exceeds maximum size");
        }
        return scriptOk(std::move(data));
#endif
    }

    inline ScriptResult<std::filesystem::path> resolveScriptFileInsideRoot(
        std::filesystem::path const& root, std::filesystem::path const& candidate
    ) {
        if (root.empty()) {
            return scriptErr<std::filesystem::path>("resources root is not configured");
        }

        std::error_code ec;
        auto path = std::filesystem::weakly_canonical(candidate, ec);
        if (ec) {
            return scriptErr<std::filesystem::path>("script path cannot be resolved: " + ec.message());
        }

        if (!pathInsideRootValue(path, root)) {
            return scriptErr<std::filesystem::path>("script path escapes resources root");
        }

        if (!std::filesystem::is_regular_file(path, ec)) {
            return scriptErr<std::filesystem::path>(
                "script file not found: " + filesystemPathString(path)
            );
        }

        return scriptOk(path);
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

    inline ScriptResult<std::filesystem::path> validateResourcePath(
        std::filesystem::path path, bool addLuauExtension = true
    ) {
        if (path.empty()) {
            return scriptErr<std::filesystem::path>("resource path is empty");
        }

        if (path.is_absolute()) {
            return scriptErr<std::filesystem::path>("resource path must not be absolute");
        }

        path = path.lexically_normal();
        if (!isFlatResourcePath(path)) {
            return scriptErr<std::filesystem::path>("resource path must be a flat resource name");
        }

        if (hasUnsupportedExtension(path)) {
            return scriptErr<std::filesystem::path>("resource path extension must be .luau");
        }

        if (addLuauExtension && !hasLuauExtension(path)) {
            path += ".luau";
        }

        if (!isFlatResourcePath(path)) {
            return scriptErr<std::filesystem::path>("resource path must be a flat resource name");
        }

        return scriptOk(std::move(path));
    }

    inline ScriptResult<std::filesystem::path> normalizeVirtualPath(std::string_view rawChunkName) {
        if (rawChunkName.empty()) {
            return scriptErr<std::filesystem::path>("chunk name is empty");
        }

        std::string text(rawChunkName);
        if (geode::utils::string::startsWith(text, "@")) {
            text.erase(text.begin());
        }

        if (text.empty()) {
            return scriptErr<std::filesystem::path>("chunk name is empty");
        }

        auto validated = validateResourcePath(std::filesystem::path(text));
        if (validated.isErr()) {
            auto const& message = validated.unwrapErr();
            if (message == "resource path is empty") {
                return scriptErr<std::filesystem::path>("chunk name is empty");
            }
            if (message == "resource path must not be absolute") {
                return scriptErr<std::filesystem::path>("chunk name must not be absolute");
            }
            if (message == "resource path must be a flat resource name") {
                return scriptErr<std::filesystem::path>("chunk name must be a flat resource name");
            }
            if (message == "resource path extension must be .luau") {
                return scriptErr<std::filesystem::path>("chunk name extension must be .luau");
            }
            return scriptErr<std::filesystem::path>(message);
        }

        return validated;
    }

    inline ScriptResult<std::filesystem::path> canonicalRoot(std::filesystem::path const& resourcesRoot) {
        if (resourcesRoot.empty()) {
            return scriptErr<std::filesystem::path>("resources root is empty");
        }

        std::error_code ec;
        auto root = std::filesystem::weakly_canonical(resourcesRoot, ec);
        if (ec) {
            return scriptErr<std::filesystem::path>(
                "resources root cannot be resolved: " + ec.message()
            );
        }

        if (!std::filesystem::is_directory(root, ec)) {
            return scriptErr<std::filesystem::path>(
                "resources root is not a directory: " + filesystemPathString(root)
            );
        }

        return scriptOk(root);
    }

    inline ScriptResult<std::filesystem::path> resolveInsideRoot(
        std::filesystem::path const& root, std::string_view relative
    ) {
        if (relative.empty()) {
            return scriptErr<std::filesystem::path>("path is empty");
        }

        std::filesystem::path rel(relative);
        if (rel.is_absolute()) {
            return scriptErr<std::filesystem::path>("path must be relative");
        }

        auto canonRoot = canonicalRoot(root);
        if (canonRoot.isErr()) {
            return scriptErr<std::filesystem::path>(canonRoot.unwrapErr());
        }

        std::error_code ec;
        auto resolved = std::filesystem::weakly_canonical(canonRoot.unwrap() / rel, ec);
        if (ec) {
            return scriptErr<std::filesystem::path>("path cannot be resolved: " + ec.message());
        }

        if (!pathInsideRootValue(resolved, canonRoot.unwrap())) {
            return scriptErr<std::filesystem::path>("path escapes the root directory");
        }

        return scriptOk(resolved);
    }
} // namespace luax
