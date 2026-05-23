#include <imes.luauapi/LuauAPI.hpp>

#include "lua/Runtime.hpp"

#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace {
    geode::Result<void> requireMainThread() {
        if (!luax::Runtime::isMainThread()) {
            return geode::Err("luau api must be called on the main thread");
        }
        return geode::Ok();
    }

    std::optional<std::string> readFileToString(std::filesystem::path const& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return std::nullopt;
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
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

        auto root = rootResult.unwrap();
        std::error_code ec;
        auto path = std::filesystem::weakly_canonical(root / relativePath, ec);
        if (ec) {
            return geode::Err("script path cannot be resolved: " + ec.message());
        }

        if (!pathInsideRoot(path, root)) {
            return geode::Err("script path escapes resources root");
        }

        if (!std::filesystem::is_regular_file(path, ec)) {
            return geode::Err("script file not found: " + normalizedPathString(path));
        }

        auto source = readFileToString(path);
        if (!source) {
            return geode::Err("script file cannot be read: " + normalizedPathString(path));
        }

        auto rel = std::filesystem::relative(path, root, ec);
        auto chunkName = "@" + normalizedPathString(ec ? relativePath : rel);
        return runScript(root, *source, chunkName, deadlineMs);
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

        auto& runtime = luax::Runtime::instance();
        runtime.setResourcesRoot(rootResult.unwrap());

        if (!runtime.runScript(source, chunkName, deadlineMs)) {
            return geode::Err("luau script failed: " + std::string(chunkName));
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
