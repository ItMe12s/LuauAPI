#include <imes.luauapi/LuauAPI.hpp>

#include "lua/Config.hpp"
#include "lua/Runtime.hpp"

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace {
    geode::Result<void> requireMainThread() {
        if (!luax::Runtime::isMainThread()) {
            return geode::Err("luau api must be called on the main thread");
        }
        return geode::Ok();
    }

    std::optional<std::string> readFileToString(std::filesystem::path const& path) {
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (ec || size > luax::kMaxScriptBytes) return std::nullopt;

        std::ifstream file(path, std::ios::binary);
        if (!file) return std::nullopt;
        std::ostringstream buffer;
        buffer << file.rdbuf();
        auto contents = buffer.str();
        if (contents.size() > luax::kMaxScriptBytes) return std::nullopt;
        return contents;
    }

    std::string normalizedPathString(std::filesystem::path const& path) {
        auto out = path.generic_string();
        for (auto& c : out) {
            if (c == '\\') c = '/';
        }
        return out;
    }

    bool escapesRoot(std::filesystem::path const& rel) {
        auto s = rel.generic_string();
        return rel.empty() || s == ".." || s.rfind("../", 0) == 0;
    }

    bool pathInsideRoot(std::filesystem::path const& path, std::filesystem::path const& root) {
        std::error_code ec;
        auto rel = std::filesystem::relative(path, root, ec);
        return !ec && !escapesRoot(rel);
    }

    bool hasLuauExtension(std::filesystem::path const& path) {
        return path.extension() == ".luau";
    }

    bool hasUnsupportedExtension(std::filesystem::path const& path) {
        auto ext = path.extension();
        return !ext.empty() && ext != ".luau";
    }

    bool isFlatResourcePath(std::filesystem::path const& path) {
        auto normalized = path.lexically_normal();
        return normalized == normalized.filename()
            && normalized != "."
            && normalized != ".."
            && !normalized.empty();
    }

    geode::Result<std::filesystem::path> normalizeVirtualPath(std::string_view rawChunkName) {
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

    geode::Result<std::filesystem::path> canonicalRoot(std::filesystem::path const& resourcesRoot) {
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

namespace imes::luauapi {
    geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot,
        std::filesystem::path const& relativePath,
        int deadlineMs
    ) {
        auto threadResult = requireMainThread();
        if (threadResult.isErr()) {
            return geode::Err(threadResult.unwrapErr());
        }

        auto rootResult = canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return geode::Err(rootResult.unwrapErr());
        }

        if (relativePath.empty()) {
            return geode::Err("relative path is empty");
        }

        if (relativePath.is_absolute()) {
            return geode::Err("relative path must not be absolute");
        }

        auto flatPathResult = normalizeVirtualPath(normalizedPathString(relativePath));
        if (flatPathResult.isErr()) {
            return geode::Err("relative path must be a flat .luau resource name");
        }
        auto flatPath = flatPathResult.unwrap();

        auto root = rootResult.unwrap();
        std::error_code ec;
        auto path = std::filesystem::weakly_canonical(root / flatPath, ec);
        if (ec) {
            return geode::Err("script path cannot be resolved: " + ec.message());
        }

        if (!pathInsideRoot(path, root)) {
            return geode::Err("script path escapes resources root");
        }

        if (!std::filesystem::is_regular_file(path, ec)) {
            return geode::Err("script file not found: " + normalizedPathString(path));
        }

        std::error_code sizeEc;
        auto fileSize = std::filesystem::file_size(path, sizeEc);
        if (!sizeEc && fileSize > luax::kMaxScriptBytes) {
            return geode::Err("script file exceeds maximum size");
        }

        auto source = readFileToString(path);
        if (!source) {
            return geode::Err("script file cannot be read: " + normalizedPathString(path));
        }

        auto rel = std::filesystem::relative(path, root, ec);
        auto chunkPath = ec ? flatPath : rel;
        return runScript(root, *source, normalizedPathString(chunkPath), deadlineMs);
    }

    geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot,
        std::string_view source,
        std::string_view chunkName,
        int deadlineMs
    ) {
        auto threadResult = requireMainThread();
        if (threadResult.isErr()) {
            return geode::Err(threadResult.unwrapErr());
        }

        auto rootResult = canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return geode::Err(rootResult.unwrapErr());
        }

        auto chunkResult = normalizeVirtualPath(chunkName);
        if (chunkResult.isErr()) {
            return geode::Err(chunkResult.unwrapErr());
        }

        auto root = rootResult.unwrap();
        auto chunkPath = chunkResult.unwrap();
        auto chunk = "@" + normalizedPathString(chunkPath);

        if (source.size() > luax::kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }

        auto& runtime = luax::Runtime::instance();
        if (!runtime.ready()) {
            auto const& err = runtime.lastError();
            return geode::Err(!err.empty() ? err : "luau runtime not ready");
        }

        luax::Runtime::ResourcesRootScope rootScope(runtime, root);

        if (!runtime.runScript(source, chunk, deadlineMs)) {
            auto const& err = runtime.lastError();
            if (!err.empty()) {
                return geode::Err(err);
            }
            return geode::Err("luau script failed: " + chunk);
        }

        return geode::Ok();
    }

    bool isReady() {
        return luax::Runtime::isInitialized();
    }

    std::size_t memoryUsage() {
        if (!luax::Runtime::isMainThread()) return 0;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->memoryUsage() : 0;
    }

    std::size_t memoryLimit() {
        if (!luax::Runtime::isMainThread()) return 0;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->memoryLimit() : 0;
    }

    bool codegenEnabled() {
        if (!luax::Runtime::isMainThread()) return false;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime && runtime->codegenEnabled();
    }
}
