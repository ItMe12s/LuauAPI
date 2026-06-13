#pragma once

#include "render3d/gpu/GpuTypes.hpp"

#include <cstdint>
#include <unordered_map>

namespace luax::render3d {

    class MeshAsset;
    struct TextureAsset;

    class Renderer3DMeshCache {
    public:
        GpuMesh* ensureGpuMesh(std::uint64_t meshId, MeshAsset const& meshAsset);
        unsigned int ensureGpuTexture(std::uint64_t textureId, TextureAsset const& textureAsset);

        void releaseMeshGpu(std::uint64_t meshId);
        void releaseTextureGpu(std::uint64_t textureId);
        void destroyAllGpuResources();
        void clear();

    private:
        void deleteGpuPrimitive(GpuPrimitive& primitive);
        void deleteGpuMesh(GpuMesh& mesh);

        std::unordered_map<std::uint64_t, GpuMesh> m_gpuMeshes;
        std::unordered_map<std::uint64_t, unsigned int> m_gpuTextures;
    };

} // namespace luax::render3d
