#include "bindings/imgui/ImGuiHost.hpp"

#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "framework/usertype/DeferredRelease.hpp"

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
#include <imgui.h>

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

        ImGuiCocos::get()
            .setup([] {
                float const density = geode::utils::getDisplayFactor();
                if (density <= 1.f) return;

                ImFontConfig cfg;
                cfg.RasterizerDensity = density;
                ImGui::GetIO().Fonts->Clear();
                ImGui::GetIO().Fonts->AddFontDefault(&cfg);
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
