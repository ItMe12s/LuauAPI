#include "bindings/imgui/ImGuiHost.hpp"

#include "bindings/imgui/ImGuiDrawScheduler.hpp"

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
} // namespace luax
