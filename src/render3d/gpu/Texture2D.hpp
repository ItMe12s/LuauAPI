#pragma once

#include <cstdint>

namespace luax::render3d {

    struct ImageData;

    enum class TextureWrapMode {
        Repeat,
        ClampToEdge,
    };

    unsigned int uploadRgbaTexture2D(
        int width, int height, std::uint8_t const* rgba,
        TextureWrapMode wrap = TextureWrapMode::Repeat, bool leaveBound = false
    );

    unsigned int uploadRgbaTexture2D(
        ImageData const& image, TextureWrapMode wrap = TextureWrapMode::Repeat, bool leaveBound = false
    );

} // namespace luax::render3d
