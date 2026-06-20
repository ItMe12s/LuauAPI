#include "render3d/gpu/GlUtil.hpp"
#include "render3d/viewport/CCViewportFrame.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <imgui-cocos.hpp>

namespace {
    bool s_gpuFeaturesDisabled = false;
    bool s_restartWarningShown = false;

    void showGpuFeaturesRestartWarning() {
        if (!s_gpuFeaturesDisabled || s_restartWarningShown) {
            return;
        }
        s_restartWarningShown = true;
        geode::createQuickPopup(
            "LuauAPI Beta",
            "All GPU features/APIs have been disabled.\nPlease restart your game to keep "
            "Fullscreen/Windowed mode.\nSorry for the inconvenience!",
            "OK",
            nullptr,
            360.0f,
            [](FLAlertLayer*, bool) {},
            true,
            true
        );
    }
} // namespace

namespace luax::render3d {

    bool gpuFeaturesDisabled() {
        return s_gpuFeaturesDisabled;
    }

    void disableGpuFeaturesForSession() {
        if (s_gpuFeaturesDisabled) {
            return;
        }
        s_gpuFeaturesDisabled = true;
        ImGuiCocos::get().destroy(true);
        abandonLiveViewports();
    }

} // namespace luax::render3d

$on_game(TexturesUnloaded) {
    luax::render3d::bumpGlContextGeneration();
    luax::render3d::disableGpuFeaturesForSession();
}

class $modify(LuauAPIMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }
        geode::queueInMainThread(showGpuFeaturesRestartWarning);
        return true;
    }
};

#ifdef GEODE_IS_WINDOWS
class $modify(LuauAPICCEGLView, cocos2d::CCEGLView) {
    void toggleFullScreen(bool value, bool borderless, bool fix) {
        luax::render3d::disableGpuFeaturesForSession();
        cocos2d::CCEGLView::toggleFullScreen(value, borderless, fix);
    }
};
#endif
