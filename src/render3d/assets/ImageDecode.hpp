#pragma once

#include <Geode/Result.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace luax::render3d {

    struct ImageData {
        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> rgba;
    };

    geode::Result<ImageData> decodeImageRgba8(std::span<std::uint8_t const> encodedBytes);

    bool isJpegMagic(std::span<std::uint8_t const> bytes);

} // namespace luax::render3d
