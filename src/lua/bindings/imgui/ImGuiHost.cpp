#include "ImGuiHost.hpp"

#include "ImGuiDrawScheduler.hpp"

#include <imgui-cocos.hpp>

namespace luax {
    namespace {
        bool s_initialized = false;
    }

    void initImGuiHost() {
        if (s_initialized) return;
        s_initialized = true;

        ImGuiCocos::get()
            .setup([] {})
            .draw([] {
                ImGuiDrawScheduler::get().drawAll();
            });
    }

    void shutdownImGuiHost() {
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
}
