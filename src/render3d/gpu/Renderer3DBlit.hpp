#pragma once

#include "render3d/gpu/Renderer3DPrograms.hpp"

namespace luax::render3d {

    void drawCompositeQuad(
        Renderer3DPrograms& programs, unsigned int colorTexture, float width, float height
    );

} // namespace luax::render3d
