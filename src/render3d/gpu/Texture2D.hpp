#pragma once

#include <cstdint>
#include <span>

namespace luax::render3d {

    struct ImageData;

    unsigned int uploadRgbaTexture2D(std::span<std::uint8_t const> rgba, int width, int height);

    unsigned int uploadRgbaTexture2D(ImageData const& image);

    unsigned int allocFramebufferTexture(int width, int height);

} // namespace luax::render3d
