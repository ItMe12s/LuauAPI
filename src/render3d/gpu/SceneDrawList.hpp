#pragma once

#include "render3d/gpu/GpuTypes.hpp"
#include "render3d/types/SceneTypes.hpp"

#include <cstdint>
#include <functional>
#include <glm/mat4x4.hpp>
#include <map>
#include <vector>

namespace luax::render3d {

    struct Frustum;
    class MeshAsset;
    struct TextureAsset;

    struct SceneDrawItem {
        GpuPrimitive const* prim = nullptr;
        GpuMesh const* texSource = nullptr;
        TextureAsset const* textureAsset = nullptr;
        std::uint64_t textureId = 0;
        int imageIndex = -1;
        unsigned int boundTexture = 0;
        glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec3 tint{1.0f, 1.0f, 1.0f};
        glm::mat4 model{1.0f};
        float viewDepth = 0.0f;
        int alphaMode = 0;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
    };

    struct SceneDrawLists {
        std::vector<SceneDrawItem> opaque;
        std::vector<SceneDrawItem> blend;
    };

    using GpuMeshResolver = std::function<GpuMesh*(std::uint64_t meshId, MeshAsset const& mesh)>;

    using TextureResolver =
        std::function<unsigned int(std::uint64_t textureId, TextureAsset const& texture)>;

    float shaderAlphaCutoff(int alphaMode, float alphaCutoff);

    bool sameInstancedBatch(SceneDrawItem const& a, SceneDrawItem const& b);

    void sortOpaqueDrawItems(std::vector<SceneDrawItem>& items);

    void sortBlendDrawItems(std::vector<SceneDrawItem>& items);

    unsigned int resolveSceneDrawTexture(
        SceneDrawItem const& item, TextureResolver const& resolveTexture, int selfColorTexture
    );

    SceneDrawLists buildSceneDrawLists(
        std::map<int, ViewportInstance> const& instances, glm::mat4 const& view,
        Frustum const& frustum, GpuMeshResolver const& resolveGpuMesh
    );

} // namespace luax::render3d
