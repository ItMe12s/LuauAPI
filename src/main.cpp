#include "LuauAPI.hpp"
#include "bindings/imgui/ImGuiHost.hpp"
#include "core/Runtime.hpp"
#include "diagnostics/BoundaryRecorder.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/Mod.hpp>
#include <thread>

namespace {
    constexpr char const* kBootstrapScript = "luauapi_Bootstrap.luau";

    void applyCrashSidecarSetting() {
        bool enabled = geode::Mod::get()->getSettingValue<bool>("enable-crash-sidecar");
        luax::diag::setRecordingEnabled(enabled);
    }
} // namespace

$on_mod(Loaded) {
    luax::Runtime::setMainThreadId(std::this_thread::get_id());
    applyCrashSidecarSetting();
    geode::listenForSettingChanges<bool>("enable-crash-sidecar", [](bool value) {
        luax::diag::setRecordingEnabled(value);
    });
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
