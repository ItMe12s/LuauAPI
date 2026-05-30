#include "LuauAPI.hpp"
#include "lua/bindings/imgui/ImGuiHost.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>

#include <thread>

$execute {
    luax::Runtime::setMainThreadId(std::this_thread::get_id());
}

$on_mod(Loaded) {
    luax::Runtime::getOrCreate();

    if (geode::Mod::get()->getSettingValue<bool>("enable-executor")) {
        auto res = imes::luauapi::runFile(
            geode::Mod::get()->getResourcesDir(), "executor_Bootstrap.luau");
        if (res.isErr()) {
            geode::log::error("[executor] failed to load: {}", res.unwrapErr());
        }
    }
}

$on_game(Exiting) {
    luax::shutdownImGuiHost();
    luax::Runtime::shutdown();
}
