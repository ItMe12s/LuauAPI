#pragma once

#include <cstdint>
#include <glm/vec4.hpp>
#include <memory>

namespace luax::render3d {

    class MeshAsset;
    struct TextureAsset;

    struct Material {
        glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        std::shared_ptr<MeshAsset> sourceMesh{};
        std::uint64_t sourceMeshId = 0;
        std::shared_ptr<TextureAsset> texture{};
        std::uint64_t textureId = 0;
        int imageIndex = -1;
        int alphaMode = 0;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
    };

} // namespace luax::render3d
