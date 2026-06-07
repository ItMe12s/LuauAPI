#include "LuauAPI.hpp"
#include "lua/bindings/imgui/ImGuiHost.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <thread>

$execute {
    luax::Runtime::setMainThreadId(std::this_thread::get_id());
}

namespace {
    constexpr char const* kBootstrapScript = "Bootstrap.luau";
}

$on_mod(Loaded) {
    luax::Runtime::getOrCreate();

    auto result = imes::luauapi::runFile(geode::Mod::get()->getResourcesDir(), kBootstrapScript);
    if (result.isErr()) {
        geode::log::error("LuauAPI Bootstrap script failed: {}", result.unwrapErr());
    }
}

$on_game(Exiting) {
    luax::shutdownImGuiHost();
    luax::Runtime::shutdown();
}
