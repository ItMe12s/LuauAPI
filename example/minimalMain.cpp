#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

using namespace geode::prelude;
namespace lua = imes::luauapi;

$on_mod(Loaded) {
    queueInMainThread([] {
        constexpr char const script[] = "Bootstrap.luau";

        auto result = lua::runFile(Mod::get()->getResourcesDir(), script);
        if (result.isErr()) {
            log::error("{} failed: {}", script, result.unwrapErr());
            auto lastError = lua::lastError();
            if (!lastError.empty()) {
                log::error("lastError: {}", lastError);
            }
        }
    });
}
