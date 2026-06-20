#include "bindings/imgui/ImGuiHost.hpp"

#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "bindings/imgui/ImGuiFontRegistry.hpp"
#include "framework/usertype/DeferredRelease.hpp"
#include "render3d/gpu/GlUtil.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <imgui-cocos.hpp>

namespace luax {
    namespace {
        bool s_initialized = false;
        bool s_pendingInit = false;
    } // namespace

    bool imguiHostIsInitialized() {
        return s_initialized && ImGuiCocos::get().isInitialized();
    }

    void imguiHostRequestReload() {
        if (render3d::gpuFeaturesDisabled()) {
            return;
        }
        if (imguiHostIsInitialized()) {
            ImGuiCocos::get().reload();
        }
    }

    void initImGuiHost() {
        if (render3d::gpuFeaturesDisabled() || s_initialized) return;

        auto* director = cocos2d::CCDirector::sharedDirector();
        if (!director || !director->getOpenGLView()) {
            if (s_pendingInit) return;
            s_pendingInit = true;
            geode::queueInMainThread([] {
                if (!s_pendingInit) return;
                s_pendingInit = false;
                initImGuiHost();
            });
            return;
        }

        ImGuiCocos::get().setDisplayScale(
            static_cast<float>(geode::Mod::get()->getSettingValue<double>("imgui-scale"))
        );

        static bool s_scaleListenerRegistered = false;
        if (!s_scaleListenerRegistered) {
            s_scaleListenerRegistered = true;
            geode::listenForSettingChanges<double>("imgui-scale", [](double value) {
                ImGuiCocos::get().setDisplayScale(static_cast<float>(value));
            });
        }

        ImGuiCocos::get()
            .setup([] {
                imguiFontRebuildAtlas();
            })
            .draw([] {
                drainDeferredReleases();
                ImGuiDrawScheduler::get().drawAll();
            });
        s_initialized = true;
    }

    void shutdownImGuiHost() {
        s_pendingInit = false;
        ImGuiDrawScheduler::get().clear();
        imguiFontClear();
        if (s_initialized && ImGuiCocos::get().isInitialized()) {
            ImGuiCocos::get().destroy();
        }
        s_initialized = false;
    }

    void imguiHostSetVisible(bool visible) {
        if (render3d::gpuFeaturesDisabled()) return;
        ImGuiCocos::get().setVisible(visible);
    }

    void imguiHostToggle() {
        if (render3d::gpuFeaturesDisabled()) return;
        ImGuiCocos::get().toggle();
    }

    bool imguiHostIsVisible() {
        if (render3d::gpuFeaturesDisabled()) {
            return false;
        }
        return ImGuiCocos::get().isVisible();
    }
} // namespace luax
