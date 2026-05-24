#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/LuauAPI.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    auto result = imes::luauapi::runFile(
        Mod::get()->getResourcesDir(),
        "Bootstrap.luau",
        250
    );

    if (result.isErr()) {
        log::error("Bootstrap.luau failed: {}", result.unwrapErr());
    }
}
