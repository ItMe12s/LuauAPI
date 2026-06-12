#pragma once

#include "render3d/MeshAsset.hpp"

#include <cstdint>
#include <span>

namespace luax::render3d {

    LoadResult<ImageData> decodeImageRgba8(std::span<std::uint8_t const> encodedBytes);

} // namespace luax::render3d
