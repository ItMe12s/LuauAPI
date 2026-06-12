#include "render3d/gpu/VertexLayout.hpp"

#include "render3d/gpu/GlUtil.hpp"

#include <cstddef>

namespace luax::render3d {

    namespace {
        int const kInterleavedVertexStride = static_cast<int>(sizeof(InterleavedVertex));
    } // namespace

    void setupInterleavedVertexAttribs() {
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, kInterleavedVertexStride, reinterpret_cast<void*>(0)
        );
        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE, kInterleavedVertexStride, reinterpret_cast<void*>(3 * sizeof(float))
        );
        glVertexAttribPointer(
            2, 2, GL_FLOAT, GL_FALSE, kInterleavedVertexStride, reinterpret_cast<void*>(6 * sizeof(float))
        );
    }

    void resetInstanceAttribs() {
#if defined(GLEW_VERSION)
        for (unsigned int location = 3; location <= 7; ++location) {
            glVertexAttribDivisor(location, 0);
            glDisableVertexAttribArray(location);
        }
#endif
    }

    void setupInstanceAttribs(unsigned int instanceVbo) {
#if defined(GLEW_VERSION)
        int const stride = static_cast<int>(sizeof(GpuInstanceData));
        glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
        for (unsigned int column = 0; column < 4; ++column) {
            unsigned int const location = 3 + column;
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(
                location,
                4,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<void*>(column * sizeof(glm::vec4))
            );
            glVertexAttribDivisor(location, 1);
        }
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(
            7, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(GpuInstanceData, tint))
        );
        glVertexAttribDivisor(7, 1);
#endif
    }

    void uploadGpuPrimitiveVertexLayout(unsigned int& vao, unsigned int vbo, unsigned int ibo) {
        vao = genVao();
        if (vao == 0) {
            return;
        }

        bindVao(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        setupInterleavedVertexAttribs();
        bindVao(0);
    }

} // namespace luax::render3d
