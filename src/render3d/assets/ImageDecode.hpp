#pragma once

#include "render3d/assets/MeshAsset.hpp"

#include <cstdint>
#include <span>

namespace luax::render3d {

    std::expected<ImageData, std::string> decodeImageRgba8(std::span<std::uint8_t const> encodedBytes);

} // namespace luax::render3d
