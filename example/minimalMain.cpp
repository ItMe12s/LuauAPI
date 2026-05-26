#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

using namespace geode::prelude;
namespace lua = imes::luauapi;

$on_mod(Loaded) {
    queueInMainThread([] {
        auto result = lua::runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau", 250);
        if (result.isErr()) {
            log::error("Bootstrap.luau failed: {}", result.unwrapErr());
            log::error("lastError: {}", lua::lastError());
        }
    });
}
