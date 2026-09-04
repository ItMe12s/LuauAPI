#pragma once

#include "render3d/gpu/GpuTypes.hpp"
#include "render3d/types/Material.hpp"
#include "render3d/types/SceneTypes.hpp"

#include <cstdint>
#include <functional>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>

namespace luax::render3d {

    struct Frustum;
    class MeshAsset;
    struct TextureAsset;

    struct SceneDrawItem {
        GpuPrimitive const* prim = nullptr;
        GpuMesh const* texSource = nullptr;
        TextureAsset const* textureAsset = nullptr;
        int imageIndex = -1;
        unsigned int boundTexture = 0;
        glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec3 tint{1.0f, 1.0f, 1.0f};
        glm::mat4 model{1.0f};
        float viewDepth = 0.0f;
        AlphaMode alphaMode = AlphaMode::Opaque;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
    };

    struct SceneDrawLists {
        std::vector<SceneDrawItem> opaque;
        std::vector<SceneDrawItem> blend;
    };

    using GpuMeshResolver = std::function<GpuMesh*(MeshAsset const& mesh)>;

    using TextureResolver = std::function<unsigned int(TextureAsset const& texture)>;

    float shaderAlphaCutoff(AlphaMode alphaMode, float alphaCutoff);

    void sortOpaqueDrawItems(std::vector<SceneDrawItem>& items);

    void sortBlendDrawItems(std::vector<SceneDrawItem>& items);

    unsigned int resolveSceneDrawTexture(
        SceneDrawItem const& item, TextureResolver& resolveTexture, int selfColorTexture
    );

    SceneDrawLists buildSceneDrawLists(
        std::unordered_map<int, ViewportInstance> const& instances, glm::mat4 const& view,
        Frustum const& frustum, GpuMeshResolver& resolveGpuMesh
    );

} // namespace luax::render3d
