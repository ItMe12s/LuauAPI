#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

using namespace geode::prelude;
namespace lua = imes::luauapi;

$on_mod(Loaded) {
    // Log diagnostics about the shared VM.
    log::info("LuauAPI codegen: {}", lua::codegenEnabled() ? "on" : "off");
    log::info("LuauAPI memory: {} / {} bytes",
        lua::memoryUsage(), lua::memoryLimit());
    // runFileAsync compiles off-thread, then executes on the main thread.
    // Prefer this over runFile (sync) to avoid hitching during load.
    lua::runFileAsync(
        Mod::get()->getResourcesDir(),
        "Bootstrap.luau",
        250 // CPU deadline in ms (default). Set <= 0 to disable budget.
    ).listen(
        [](geode::Result<void>* result) {
            if (result && result->isErr()) {
                log::error("Bootstrap.luau failed: {}", result->unwrapErr());
                // lua::lastError() has the latest sync error string.
                // Read it before your next LuauAPI call because it gets overwritten.
                log::error("lastError: {}", lua::lastError());
            }
        },
        [](auto*) {},
        []() {
            log::warn("Bootstrap.luau cancelled");
        }
    );

    // Sync alternative (blocks the main thread during compile and execute):
    //   auto result = lua::runFile(
    //       Mod::get()->getResourcesDir(), "Bootstrap.luau");
    //   if (result.isErr()) log::error("{}", result.unwrapErr());
}
