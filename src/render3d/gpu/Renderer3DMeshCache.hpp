#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace luax::render3d {

    struct MeshAsset;
    struct TextureAsset;

    struct GpuPrimitive {
        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ibo = 0;
        unsigned int indexCount = 0;
        int materialIndex = -1;
    };

    struct GpuMesh {
        std::vector<GpuPrimitive> primitives;
        std::vector<unsigned int> textures;
    };

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
