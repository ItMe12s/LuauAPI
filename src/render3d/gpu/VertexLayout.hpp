#pragma once

#include <cstddef>
#include <cstdint>

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

    static_assert(sizeof(InterleavedVertex) == 32, "InterleavedVertex must be 32 bytes");

    void setupInterleavedVertexAttribs();

    void bindInterleavedBuffers(std::uint32_t vbo, std::uint32_t ibo);

} // namespace luax::render3d
