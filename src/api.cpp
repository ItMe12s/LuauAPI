#include "core/Runtime.hpp"
#include "require/PathSandbox.hpp"

#include <LuauAPI.hpp>

#if !defined(LUAUAPI_HOST_TESTS)
    #include <Geode/utils/async.hpp>
#endif

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace imes::luauapi {
    geode::Result<void> resolveAsyncMainThreadResult(std::optional<geode::Result<void>> const& result);
}

namespace {
    struct PreparedRunFile {
        std::filesystem::path root;
        std::string source;
        std::string chunk;
    };

    struct PreparedRunScript {
        std::filesystem::path root;
        std::string source;
        std::string chunk;
    };

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
        std::filesystem::path const& root, std::string source, std::string chunk, int deadlineMs
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

        auto result = runtime->runScript(source, chunk, deadlineMs);
        if (result.isErr()) {
            return geode::Err(result.unwrapErr());
        }

        return geode::Ok();
    }

    geode::Result<void> resolveRunFilePath(
        std::filesystem::path const& resourcesRoot, std::filesystem::path const& relativePath,
        std::filesystem::path& outPath, std::filesystem::path& outRoot
    ) {
        auto rootResult = luax::canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return geode::Err(rootResult.unwrapErr());
        }

        auto flatPathResult = luax::validateResourcePath(relativePath);
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

    geode::Result<PreparedRunFile> prepareRunFile(
        std::filesystem::path const& resourcesRoot, std::filesystem::path const& relativePath
    ) {
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

        return geode::Ok(
            PreparedRunFile{std::move(root), std::move(sourceResult.unwrap()), chunkResult.unwrap()}
        );
    }

    geode::Result<PreparedRunScript> prepareRunScript(
        std::filesystem::path const& resourcesRoot, std::string_view chunkName,
        std::string_view sourceBytes
    ) {
        auto rootResult = luax::canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return geode::Err(rootResult.unwrapErr());
        }

        auto chunkResult = prepareChunkName(chunkName);
        if (chunkResult.isErr()) {
            return geode::Err(chunkResult.unwrapErr());
        }

        if (sourceBytes.size() > luax::kMaxScriptBytes) {
            return geode::Err("script exceeds maximum size");
        }

        return geode::Ok(
            PreparedRunScript{rootResult.unwrap(), std::string(sourceBytes), chunkResult.unwrap()}
        );
    }

#if !defined(LUAUAPI_HOST_TESTS)
    arc::Future<geode::Result<void>> executeOnMainAsync(
        std::filesystem::path root, std::string source, std::string chunk, int deadlineMs
    ) {
        auto result =
            co_await geode::async::waitForMainThread<geode::Result<void>>([root = std::move(root),
                                                                           chunk = std::move(chunk),
                                                                           source = std::move(source),
                                                                           deadlineMs]() mutable {
                return executeScriptOnMain(root, std::move(source), chunk, deadlineMs);
            });
        co_return imes::luauapi::resolveAsyncMainThreadResult(result);
    }
#endif
} // namespace

namespace imes::luauapi {
    geode::Result<void> runFile(
        std::filesystem::path const& resourcesRoot, std::filesystem::path const& relativePath,
        int deadlineMs
    ) {
        auto threadResult = requireMainThread();
        if (threadResult.isErr()) {
            return geode::Err(threadResult.unwrapErr());
        }
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }

        auto prepared = prepareRunFile(resourcesRoot, relativePath);
        if (prepared.isErr()) {
            return geode::Err(prepared.unwrapErr());
        }

        auto run = std::move(prepared.unwrap());
        return executeScriptOnMain(
            std::move(run.root), std::move(run.source), std::move(run.chunk), deadlineMs
        );
    }

    geode::Result<void> runScript(
        std::filesystem::path const& resourcesRoot, std::string_view source,
        std::string_view chunkName, int deadlineMs
    ) {
        auto threadResult = requireMainThread();
        if (threadResult.isErr()) {
            return geode::Err(threadResult.unwrapErr());
        }
        if (luax::Runtime::isShuttingDown()) {
            return geode::Err("luau runtime shutting down");
        }

        auto prepared = prepareRunScript(resourcesRoot, chunkName, source);
        if (prepared.isErr()) {
            return geode::Err(prepared.unwrapErr());
        }

        auto run = std::move(prepared.unwrap());
        return executeScriptOnMain(
            std::move(run.root), std::move(run.source), std::move(run.chunk), deadlineMs
        );
    }

    geode::Result<void> resolveAsyncMainThreadResult(std::optional<geode::Result<void>> const& result) {
        if (!result) {
            if (luax::Runtime::isShuttingDown()) {
                return geode::Err("luau runtime shutting down");
            }
            return geode::Err("luau main-thread execution cancelled");
        }
        return *result;
    }

#if !defined(LUAUAPI_HOST_TESTS)
    arc::Future<geode::Result<void>> runFileAsync(
        std::filesystem::path resourcesRoot, std::filesystem::path relativePath, int deadlineMs
    ) {
        if (luax::Runtime::isShuttingDown()) {
            co_return geode::Err("luau runtime shutting down");
        }

        auto prepared = prepareRunFile(resourcesRoot, relativePath);
        if (prepared.isErr()) {
            co_return geode::Err(prepared.unwrapErr());
        }

        auto run = std::move(prepared.unwrap());
        co_return co_await executeOnMainAsync(
            std::move(run.root), std::move(run.source), std::move(run.chunk), deadlineMs
        );
    }

    arc::Future<geode::Result<void>> runScriptAsync(
        std::filesystem::path resourcesRoot, std::string source, std::string chunkName, int deadlineMs
    ) {
        if (luax::Runtime::isShuttingDown()) {
            co_return geode::Err("luau runtime shutting down");
        }

        auto prepared = prepareRunScript(resourcesRoot, chunkName, source);
        if (prepared.isErr()) {
            co_return geode::Err(prepared.unwrapErr());
        }

        auto run = std::move(prepared.unwrap());
        co_return co_await executeOnMainAsync(
            std::move(run.root), std::move(run.source), std::move(run.chunk), deadlineMs
        );
    }
#endif

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

    std::string lastError() {
        if (luax::Runtime::isShuttingDown()) return {};
        if (!luax::Runtime::isMainThread()) return {};
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->lastError() : std::string{};
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
} // namespace imes::luauapi
