#include "ImGuiHost.hpp"

#include "ImGuiDrawScheduler.hpp"

#include <imgui-cocos.hpp>

namespace luax {
    namespace {
        bool g_initialized = false;
    }

    void initImGuiHost() {
        if (g_initialized) return;
        g_initialized = true;

        ImGuiCocos::get()
            .setup([] {})
            .draw([] {
                ImGuiDrawScheduler::get().drawAll();
            });
    }

    void shutdownImGuiHost() {
        ImGuiDrawScheduler::get().clear();
        if (g_initialized && ImGuiCocos::get().isInitialized()) {
            ImGuiCocos::get().destroy();
        }
        g_initialized = false;
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
