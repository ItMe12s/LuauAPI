#pragma once

#include "render3d/gpu/GpuTypes.hpp"

#include <unordered_map>

namespace luax::render3d {

    class MeshAsset;
    struct TextureAsset;

    class Renderer3DMeshCache {
    public:
        GpuMesh* ensureGpuMesh(MeshAsset const& meshAsset);
        unsigned int ensureGpuTexture(TextureAsset const& textureAsset);

        void releaseMeshGpu(MeshAsset const* mesh);
        void releaseTextureGpu(TextureAsset const* texture);
        void destroyAllGpuResources();
        void clear();

    private:
        void deleteGpuPrimitive(GpuPrimitive& primitive);
        void deleteGpuMesh(GpuMesh& mesh);

        std::unordered_map<MeshAsset const*, GpuMesh> m_gpuMeshes;
        std::unordered_map<TextureAsset const*, unsigned int> m_gpuTextures;
        unsigned m_glContextGeneration = 0;
    };

} // namespace luax::render3d
