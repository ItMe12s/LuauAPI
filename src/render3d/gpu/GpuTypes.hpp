#pragma once

#include <cstdint>
#include <vector>

namespace luax::render3d {

    struct GpuPrimitive {
        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ibo = 0;
        unsigned int indexCount = 0;
        int materialIndex = -1;
    };

    struct GpuMesh {
        std::vector<GpuPrimitive> primitives;
        std::vector<unsigned int> textures;
    };

} // namespace luax::render3d
