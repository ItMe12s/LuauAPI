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

    inline bool isDrawableGpuPrimitive(GpuPrimitive const& primitive) {
        return primitive.vbo != 0 && primitive.ibo != 0 && primitive.indexCount != 0;
    }

    inline bool hasDrawableGpuPrimitive(GpuMesh const& mesh) {
        for (auto const& primitive : mesh.primitives) {
            if (isDrawableGpuPrimitive(primitive)) {
                return true;
            }
        }
        return false;
    }

} // namespace luax::render3d
