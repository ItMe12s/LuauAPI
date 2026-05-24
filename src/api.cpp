#include <imes.luauapi/LuauAPI.hpp>

#include "lua/PathSandbox.hpp"
#include "lua/Runtime.hpp"

#include <Geode/utils/async.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace {
    using RunTask = geode::Task<geode::Result<void>>;

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
        std::error_code ec;
        auto path = std::filesystem::weakly_canonical(outRoot / flatPath, ec);
        if (ec) {
            return geode::Err("script path cannot be resolved: " + ec.message());
        }

        if (!luax::pathInsideRoot(path, outRoot)) {
            return geode::Err("script path escapes resources root");
        }

        if (!std::filesystem::is_regular_file(path, ec)) {
            return geode::Err("script file not found: " + luax::normalizedPathString(path));
        }

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

        std::filesystem::path path;
        std::filesystem::path root;
        auto resolveResult = resolveRunFilePath(resourcesRoot, relativePath, path, root);
        if (resolveResult.isErr()) {
            return geode::Err(resolveResult.unwrapErr());
        }

        auto sourceResult = luax::readScriptFile(path);
        if (sourceResult.isErr()) {
            return geode::Err("script file cannot be read: " + luax::normalizedPathString(path));
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

    RunTask runFileAsync(
        std::filesystem::path const& resourcesRoot,
        std::filesystem::path const& relativePath,
        int deadlineMs
    ) {
        return RunTask::runWithCallback([resourcesRoot, relativePath, deadlineMs](
            auto finish,
            auto /*progress*/,
            auto hasBeenCancelled
        ) {
            if (hasBeenCancelled()) {
                finish(geode::Err("cancelled"));
                return;
            }

            std::filesystem::path path;
            std::filesystem::path root;
            auto resolveResult = resolveRunFilePath(resourcesRoot, relativePath, path, root);
            if (resolveResult.isErr()) {
                finish(geode::Err(resolveResult.unwrapErr()));
                return;
            }

            auto sourceResult = luax::readScriptFile(path);
            if (sourceResult.isErr()) {
                finish(geode::Err("script file cannot be read: " + luax::normalizedPathString(path)));
                return;
            }

            if (hasBeenCancelled()) {
                finish(geode::Err("cancelled"));
                return;
            }

            std::error_code ec;
            auto rel = std::filesystem::relative(path, root, ec);
            auto chunkPath = ec ? path.filename() : rel;
            auto chunkResult = prepareChunkName(luax::normalizedPathString(chunkPath));
            if (chunkResult.isErr()) {
                finish(geode::Err(chunkResult.unwrapErr()));
                return;
            }

            auto source = std::move(sourceResult.unwrap());
            auto chunk = chunkResult.unwrap();
            auto bytecode = luax::Runtime::compileSource(source);

            geode::queueInMainThread([finish = std::move(finish), root, chunk = std::move(chunk), bytecode = std::move(bytecode), deadlineMs]() mutable {
                if (!luax::Runtime::isMainThread()) {
                    finish(geode::Err("luau api must be called on the main thread"));
                    return;
                }

                auto& runtime = luax::Runtime::instance();
                if (!runtime.ready()) {
                    auto const& err = runtime.lastError();
                    finish(geode::Err(!err.empty() ? err : "luau runtime not ready"));
                    return;
                }

                luax::Runtime::ResourcesRootScope rootScope(runtime, root);
                finish(runtime.runBytecode(bytecode, chunk, deadlineMs));
            });
        }, "luau run file");
    }

    RunTask runScriptAsync(
        std::filesystem::path const& resourcesRoot,
        std::string source,
        std::string_view chunkName,
        int deadlineMs
    ) {
        return RunTask::runWithCallback([resourcesRoot, source = std::move(source), chunkName, deadlineMs](
            auto finish,
            auto /*progress*/,
            auto hasBeenCancelled
        ) mutable {
            if (hasBeenCancelled()) {
                finish(geode::Err("cancelled"));
                return;
            }

            auto rootResult = luax::canonicalRoot(resourcesRoot);
            if (rootResult.isErr()) {
                finish(geode::Err(rootResult.unwrapErr()));
                return;
            }

            auto chunkResult = prepareChunkName(chunkName);
            if (chunkResult.isErr()) {
                finish(geode::Err(chunkResult.unwrapErr()));
                return;
            }

            if (source.size() > luax::kMaxScriptBytes) {
                finish(geode::Err("script exceeds maximum size"));
                return;
            }

            if (hasBeenCancelled()) {
                finish(geode::Err("cancelled"));
                return;
            }

            auto root = rootResult.unwrap();
            auto chunk = chunkResult.unwrap();
            auto bytecode = luax::Runtime::compileSource(source);

            geode::queueInMainThread([finish = std::move(finish), root, chunk = std::move(chunk), bytecode = std::move(bytecode), deadlineMs]() mutable {
                if (!luax::Runtime::isMainThread()) {
                    finish(geode::Err("luau api must be called on the main thread"));
                    return;
                }

                auto& runtime = luax::Runtime::instance();
                if (!runtime.ready()) {
                    auto const& err = runtime.lastError();
                    finish(geode::Err(!err.empty() ? err : "luau runtime not ready"));
                    return;
                }

                luax::Runtime::ResourcesRootScope rootScope(runtime, root);
                finish(runtime.runBytecode(bytecode, chunk, deadlineMs));
            });
        }, "luau run script");
    }

    bool isReady() {
        return luax::Runtime::isInitialized();
    }

    RuntimeStatus status() {
        if (!luax::Runtime::isMainThread()) return RuntimeStatus::NotReady;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? runtime->status() : RuntimeStatus::NotReady;
    }

    std::string_view lastError() {
        if (!luax::Runtime::isMainThread()) return {};
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime ? std::string_view(runtime->lastError()) : std::string_view();
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

    void setMemoryLimit(std::size_t bytes) {
        if (!luax::Runtime::isMainThread()) return;
        luax::Runtime::instance().setMemoryLimit(bytes);
    }

    bool codegenEnabled() {
        if (!luax::Runtime::isMainThread()) return false;
        auto* runtime = luax::Runtime::getIfInitialized();
        return runtime && runtime->codegenEnabled();
    }
}
