#include "bindings/imgui/ImGuiHost.hpp"
#include "render3d/gpu/GlUtil.hpp"
#include "render3d/viewport/CCViewportFrame.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <imgui-cocos.hpp>

namespace luax::render3d {

    void reloadGpuAfterTextureUnload() {
        bumpGlContextGeneration();
        markGameTexturesUnloaded();
        abandonLiveViewports();
        if (ImGuiCocos::get().isInitialized()) {
            ImGuiCocos::get().reload(true);
        }
    }

} // namespace luax::render3d

$on_game(TexturesLoaded) {
    luax::render3d::markGameTexturesLoaded();
    geode::queueInMainThread([] {
        luax::initImGuiHost();
    });
}

$on_game(TexturesUnloaded) {
    luax::render3d::reloadGpuAfterTextureUnload();
}

class $modify(LuauAPIMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }
        luax::render3d::markGameTexturesLoaded();
        geode::queueInMainThread([] {
            luax::initImGuiHost();
        });
        return true;
    }
};
