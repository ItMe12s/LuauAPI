#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

using namespace geode::prelude;
namespace lua = imes::luauapi;

static void loadBootstrap() {
    if (lua::status() != lua::RuntimeStatus::Ready) {
        log::error("LuauAPI not ready (status {})", static_cast<int>(lua::status()));
        return;
    }

    auto result = lua::runFile(
        Mod::get()->getResourcesDir(),
        "Bootstrap.luau",
        250
    );
    if (result.isErr()) {
        log::error("Bootstrap.luau failed: {}", result.unwrapErr());
        log::error("lastError: {}", lua::lastError());
    } else {
        log::info("Bootstrap.luau loaded");
    }
}

$on_mod(Loaded) {
    log::info("LuauAPI codegen: {}", lua::codegenEnabled() ? "on" : "off");
    log::info("LuauAPI memory: {} / {} bytes", lua::memoryUsage(), lua::memoryLimit());

    queueInMainThread([] {
        loadBootstrap();
    });
}
