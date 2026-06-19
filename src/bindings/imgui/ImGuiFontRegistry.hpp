#pragma once

#include <cstdint>
#include <vector>

struct ImFont;

namespace luax {
    struct ImGuiFontHandle {
        std::uint64_t id = 0;
    };

    std::uint64_t imguiFontAdd(float size, std::vector<std::uint8_t> data);

    ImFont* imguiFontResolve(std::uint64_t id);

    void imguiFontRemove(std::uint64_t id);

    void imguiFontRebuildAtlas();

    void imguiFontAfterRegistryChange();

    void imguiFontClear();
} // namespace luax
