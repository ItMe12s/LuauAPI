#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <Geode/utils/async.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

#include <utility>

using namespace geode::prelude;
namespace lua = imes::luauapi;

namespace {
    geode::async::TaskHolder<geode::Result<void>> g_asyncScriptTask;

    void reportScriptResult(char const* script, geode::Result<void> result) {
        if (result.isErr()) {
            log::error("{} failed: {}", script, result.unwrapErr());
            log::error("lastError: {}", lua::lastError());
        } else {
            log::info("{} loaded", script);
        }
    }

    void loadExampleScripts() {
        if (lua::status() != lua::RuntimeStatus::Ready) {
            log::error("LuauAPI not ready (status {})", static_cast<int>(lua::status()));
            return;
        }

        reportScriptResult(
            "MainThreadBootstrap.luau",
            lua::runFile(Mod::get()->getResourcesDir(), "MainThreadBootstrap.luau", 250)
        );

        g_asyncScriptTask.spawn(
            "luau example async bootstrap",
            lua::runFileAsync(Mod::get()->getResourcesDir(), "AsyncBootstrap.luau", 250),
            [](geode::Result<void> result) {
                reportScriptResult("AsyncBootstrap.luau", std::move(result));
            }
        );
    }
}

$on_mod(Loaded) {
    log::info("LuauAPI codegen: {}", lua::codegenEnabled() ? "on" : "off");
    log::info("LuauAPI memory: {} / {} bytes", lua::memoryUsage(), lua::memoryLimit());

    queueInMainThread([] {
        loadExampleScripts();
    });
}
