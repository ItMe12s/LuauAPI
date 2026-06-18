#include "bindings/imgui/ImGuiHost.hpp"

#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "framework/usertype/DeferredRelease.hpp"

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>

namespace luax {
    namespace {
        bool s_initialized = false;
        bool s_pendingInit = false;
    } // namespace

    void initImGuiHost() {
        if (s_initialized) return;

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

        ImGuiCocos::get().setup([] {}).draw([] {
            drainDeferredReleases();
            ImGuiDrawScheduler::get().drawAll();
        });
        s_initialized = true;
    }

    void shutdownImGuiHost() {
        s_pendingInit = false;
        ImGuiDrawScheduler::get().clear();
        if (s_initialized && ImGuiCocos::get().isInitialized()) {
            ImGuiCocos::get().destroy();
        }
        s_initialized = false;
    }

    void imguiHostSetVisible(bool visible) {
        ImGuiCocos::get().setVisible(visible);
    }

    void imguiHostToggle() {
        ImGuiCocos::get().toggle();
    }

    bool imguiHostIsVisible() {
        return ImGuiCocos::get().isVisible();
    }
} // namespace luax
