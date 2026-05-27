#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <Geode/utils/async.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

#include <utility>

using namespace geode::prelude;
namespace lua = imes::luauapi;

namespace {
    constexpr char const kMainThreadScript[] = "MainThreadBootstrap.luau";
    constexpr char const kAsyncScript[] = "AsyncBootstrap.luau";

    geode::async::TaskHolder<geode::Result<void>> g_asyncScriptTask;

    void reportScriptResult(char const* script, geode::Result<void> result) {
        if (result.isErr()) {
            log::error("{} failed: {}", script, result.unwrapErr());
            auto lastError = lua::lastError();
            if (!lastError.empty()) {
                log::error("lastError: {}", lastError);
            }
        } else {
            log::info("{} loaded", script);
        }
    }

    void loadExampleScripts() {
        auto status = lua::status();
        if (status != lua::RuntimeStatus::Ready) {
            log::error("LuauAPI not ready (status {})", static_cast<int>(status));
            return;
        }

        reportScriptResult(
            kMainThreadScript,
            lua::runFile(Mod::get()->getResourcesDir(), kMainThreadScript)
        );

        g_asyncScriptTask.spawn(
            "luau example async bootstrap",
            lua::runFileAsync(Mod::get()->getResourcesDir(), kAsyncScript),
            [](geode::Result<void> result) {
                reportScriptResult(kAsyncScript, std::move(result));
            }
        );
    }
}

$on_mod(Loaded) {
    log::info("LuauAPI status: {}", static_cast<int>(lua::status()));
    log::info("LuauAPI codegen: {}", lua::codegenEnabled() ? "on" : "off");
    log::info("LuauAPI memory: {} / {} bytes", lua::memoryUsage(), lua::memoryLimit());

    queueInMainThread([] {
        loadExampleScripts();
    });
}
