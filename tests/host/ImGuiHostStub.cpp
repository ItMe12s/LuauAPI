#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "bindings/imgui/ImGuiHost.hpp"

namespace luax {
    void initImGuiHost() {}

    void shutdownImGuiHost() {
        ImGuiDrawScheduler::get().clear();
    }

    void imguiHostSetVisible(bool) {}

    void imguiHostToggle() {}

    bool imguiHostIsVisible() {
        return false;
    }

    bool imguiHostIsInitialized() {
        return false;
    }

    void imguiHostRequestReload() {}
} // namespace luax
