#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <imes.luauapi/LuauAPI.hpp>

using namespace geode::prelude;

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        static bool bootstrapLoaded = false;
        if (!bootstrapLoaded) {
            auto result = imes::luauapi::runFile(
                Mod::get()->getResourcesDir(),
                "Bootstrap.luau",
                250,
                this
            );
            if (result.isOk()) {
                bootstrapLoaded = true;
            } else {
                log::error("Bootstrap.luau failed: {}", result.unwrapErr());
            }
        }

        return true;
    }
};
