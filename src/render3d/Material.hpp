#pragma once

#include <cstdint>
#include <glm/vec4.hpp>
#include <memory>

namespace luax::render3d {

    class MeshAsset;

    struct Material {
        glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        std::shared_ptr<MeshAsset> sourceMesh{};
        std::uint64_t sourceMeshId = 0;
        int imageIndex = -1;
    };

} // namespace luax::render3d
