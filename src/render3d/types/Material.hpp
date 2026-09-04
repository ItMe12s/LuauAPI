#pragma once

#include <glm/vec4.hpp>
#include <memory>

namespace luax::render3d {

    class MeshAsset;
    struct TextureAsset;

    enum class AlphaMode : int {
        Opaque = 0,
        Mask = 1,
        Blend = 2,
    };

    struct Material {
        glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        std::shared_ptr<MeshAsset> sourceMesh{};
        std::shared_ptr<TextureAsset> texture{};
        int imageIndex = -1;
        AlphaMode alphaMode = AlphaMode::Opaque;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
    };

} // namespace luax::render3d
