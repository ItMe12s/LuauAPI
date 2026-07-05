#pragma once

#include "render3d/assets/MeshAsset.hpp"

#include <Geode/Result.hpp>
#include <cstdint>
#include <span>

namespace luax::render3d {

    geode::Result<ImageData> decodeImageRgba8(std::span<std::uint8_t const> encodedBytes);

} // namespace luax::render3d
