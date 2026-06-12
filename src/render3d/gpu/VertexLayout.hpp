#pragma once

#include <glm/glm.hpp>

namespace luax::render3d {

    struct InterleavedVertex {
        float px;
        float py;
        float pz;
        float nx;
        float ny;
        float nz;
        float u;
        float v;
    };

    struct GpuInstanceData {
        glm::mat4 model{1.0f};
        glm::vec4 tint{1.0f, 1.0f, 1.0f, 0.0f};
    };

    void setupInterleavedVertexAttribs();
    void resetInstanceAttribs();
    void setupInstanceAttribs(unsigned int instanceVbo);
    void uploadGpuPrimitiveVertexLayout(unsigned int& vao, unsigned int vbo, unsigned int ibo);

} // namespace luax::render3d
