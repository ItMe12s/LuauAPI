#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/LuauAPI.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
    imes::luauapi::runFileAsync(
        Mod::get()->getResourcesDir(),
        "Bootstrap.luau",
        250
    ).listen(
        [](geode::Result<void>* result) {
            if (result && result->isErr()) {
                log::error("Bootstrap.luau failed: {}", result->unwrapErr());
            }
        },
        [](auto*) {},
        []() {
            log::warn("Bootstrap.luau cancelled");
        }
    );
}
