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

    void bindInterleavedBuffers(std::uint32_t vbo, std::uint32_t ibo) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        setupInterleavedVertexAttribs();
    }

} // namespace luax::render3d
