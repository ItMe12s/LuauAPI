#pragma once

namespace luax {
    void initImGuiHost();

    void shutdownImGuiHost();

    void imguiHostSetVisible(bool visible);
    void imguiHostToggle();
    bool imguiHostIsVisible();
}
