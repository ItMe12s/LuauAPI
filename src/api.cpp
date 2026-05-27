#include <LuauAPI.hpp>

#include "lua/PathSandbox.hpp"
#include "lua/Runtime.hpp"

#include <Geode/utils/async.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace {
    geode::Result<void> requireMainThread() {
        if (!luax::Runtime::isMainThread()) {
            return geode::Err("luau api must be called on the main thread");
        }
        return geode::Ok();
    }

    geode::Result<std::string> prepareChunkName(std::string_view chunkName) {
        auto chunkResult = luax::normalizeVirtualPath(chunkName);
        if (chunkResult.isErr()) {
            return geode::Err(chunkResult.unwrapErr());
        }
        return geode::Ok("@" + luax::normalizedPathString(chunkResult.unwrap()));
    }

    geode::Result<void> executeScriptOnMain(
        std::filesystem::path const& root,
        std::string source,
        std::string chunk,
        int deadlineMs
    ) {
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }

        auto* runtime = luax::Runtime::getOrCreate();
        if (!runtime) {
            return geode::Err("luau runtime shutting down");
        }
        if (!runtime->ready()) {
            auto const& err = runtime->lastError();
            return geode::Err(!err.empty() ? err : "luau runtime not ready");
        }

        luax::Runtime::ResourcesRootScope rootScope(*runtime, root);

        if (!runtime->runScript(source, chunk, deadlineMs)) {
            auto const& err = runtime->lastError();
            if (!err.empty()) {
                return geode::Err(err);
            }
            return geode::Err("luau script failed: " + chunk);
        }

        return geode::Ok();
    }

    geode::Result<void> resolveRunFilePath(
        std::filesystem::path const& resourcesRoot,
        std::filesystem::path const& relativePath,
        std::filesystem::path& outPath,
        std::filesystem::path& outRoot
    ) {
        auto rootResult = luax::canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return geode::Err(rootResult.unwrapErr());
        }

        if (relativePath.empty()) {
            return geode::Err("relative path is empty");
        }

        if (relativePath.is_absolute()) {
            return geode::Err("relative path must not be absolute");
        }

        auto flatPathResult = luax::normalizeVirtualPath(luax::normalizedPathString(relativePath));
        if (flatPathResult.isErr()) {
            return geode::Err("relative path must be a flat .luau resource name");
        }
        auto flatPath = flatPathResult.unwrap();

        outRoot = rootResult.unwrap();
        auto pathResult = luax::resolveScriptFileInsideRoot(outRoot, outRoot / flatPath);
        if (pathResult.isErr()) {
            return geode::Err(pathResult.unwrapErr());
        }
        auto path = pathResult.unwrap();

        std::error_code sizeEc;
        auto fileSize = std::filesystem::file_size(path, sizeEc);
        if (!sizeEc && fileSize > luax::kMaxScriptBytes) {
            return geode::Err("script file exceeds maximum size");
        }

        outPath = path;
        return geode::Ok();
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
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }

        std::filesystem::path path;
        std::filesystem::path root;
        auto resolveResult = resolveRunFilePath(resourcesRoot, relativePath, path, root);
        if (resolveResult.isErr()) {
            return geode::Err(resolveResult.unwrapErr());
        }

        auto sourceResult = luax::readScriptFile(path);
        if (sourceResult.isErr()) {
            return geode::Err(sourceResult.unwrapErr());
        }

        std::error_code ec;
        auto rel = std::filesystem::relative(path, root, ec);
        auto chunkPath = ec ? path.filename() : rel;
        auto chunkResult = prepareChunkName(luax::normalizedPathString(chunkPath));
        if (chunkResult.isErr()) {
            return geode::Err(chunkResult.unwrapErr());
        }

        return executeScriptOnMain(root, std::move(sourceResult.unwrap()), chunkResult.unwrap(), deadlineMs);
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
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }

        auto rootResult = luax::canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return geode::Err(rootResult.unwrapErr());
        }

        auto chunkResult = prepareChunkName(chunkName);
        if (chunkResult.isErr()) {
            return geode::Err(chunkResult.unwrapErr());
        }

        if (source.size() > luax::kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }

        return executeScriptOnMain(
            rootResult.unwrap(),
            std::string(source),
            chunkResult.unwrap(),
            deadlineMs
        );
    }

    arc::Future<geode::Result<void>> runFileAsync(
        std::filesystem::path resourcesRoot,
        std::filesystem::path relativePath,
        int deadlineMs
    ) {
        if (luax::Runtime::isShuttingDown()) {
            co_return geode::Err("luau runtime shutting down");
        }

        std::filesystem::path path;
        std::filesystem::path root;
        auto resolveResult = resolveRunFilePath(resourcesRoot, relativePath, path, root);
        if (resolveResult.isErr()) {
            co_return geode::Err(resolveResult.unwrapErr());
        }

        auto sourceResult = luax::readScriptFile(path);
        if (sourceResult.isErr()) {
            co_return geode::Err(sourceResult.unwrapErr());
        }

        std::error_code ec;
        auto rel = std::filesystem::relative(path, root, ec);
        auto chunkPath = ec ? path.filename() : rel;
        auto chunkResult = prepareChunkName(luax::normalizedPathString(chunkPath));
        if (chunkResult.isErr()) {
            co_return geode::Err(chunkResult.unwrapErr());
        }

        auto source = std::move(sourceResult.unwrap());
        auto chunk = chunkResult.unwrap();

        auto result = co_await geode::async::waitForMainThread<geode::Result<void>>(
            [root = std::move(root), chunk = std::move(chunk), source = std::move(source), deadlineMs]() mutable {
                return executeScriptOnMain(root, std::move(source), chunk, deadlineMs);
            }
        );
        if (!result) {
            co_return geode::Err("luau main-thread execution cancelled");
        }
        co_return std::move(*result);
    }

    arc::Future<geode::Result<void>> runScriptAsync(
        std::filesystem::path resourcesRoot,
        std::string source,
        std::string chunkName,
        int deadlineMs
    ) {
        if (luax::Runtime::isShuttingDown()) {
            co_return geode::Err("luau runtime shutting down");
        }

        auto rootResult = luax::canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            co_return geode::Err(rootResult.unwrapErr());
        }

        auto chunkResult = prepareChunkName(chunkName);
        if (chunkResult.isErr()) {
            co_return geode::Err(chunkResult.unwrapErr());
        }

        if (source.size() > luax::kMaxScriptBytes) {
            co_return geode::Err("script exceeds maximum size");
        }

        auto root = rootResult.unwrap();
        auto chunk = chunkResult.unwrap();

        auto result = co_await geode::async::waitForMainThread<geode::Result<void>>(
            [root = std::move(root), chunk = std::move(chunk), source = std::move(source), deadlineMs]() mutable {
                return executeScriptOnMain(root, std::move(source), chunk, deadlineMs);
            }
        );
        if (!result) {
            co_return geode::Err("luau main-thread execution cancelled");
        }
        co_return std::move(*result);
    }

    bool isReady() {
        if (luax::Runtime::isShuttingDown()) return false;
        if (!luax::Runtime::isMainThread()) return false;
        return luax::Runtime::isInitialized();
    }

    RuntimeStatus status() {
        if (luax::Runtime::isShuttingDown()) return RuntimeStatus::NotReady;
        if (!luax::Runtime::isMainThread()) return RuntimeStatus::NotReady;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->status() : RuntimeStatus::NotReady;
    }

    std::string_view lastError() {
        if (luax::Runtime::isShuttingDown()) return {};
        if (!luax::Runtime::isMainThread()) return {};
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? std::string_view(runtime->lastError()) : std::string_view();
    }

    std::size_t memoryUsage() {
        if (luax::Runtime::isShuttingDown()) return 0;
        if (!luax::Runtime::isMainThread()) return 0;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->memoryUsage() : 0;
    }

    std::size_t memoryLimit() {
        if (luax::Runtime::isShuttingDown()) return 0;
        if (!luax::Runtime::isMainThread()) return 0;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->memoryLimit() : 0;
    }

    bool codegenEnabled() {
        if (luax::Runtime::isShuttingDown()) return false;
        if (!luax::Runtime::isMainThread()) return false;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime && runtime->codegenEnabled();
    }
}
