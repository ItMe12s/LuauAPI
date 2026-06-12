#pragma once

#include "render3d/types/SceneTypes.hpp"

#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

namespace luax::render3d {

    struct TextureAsset;

    class Renderer3D {
    public:
        static Renderer3D& instance();

        void renderToFramebuffer(
            unsigned int fbo, int pixelWidth, int pixelHeight, Camera3D const& camera,
            std::map<int, ViewportInstance> const& instances, RenderSettings const& settings,
            std::map<int, DebugLine> const& debugLines, bool debugBounds
        );

        void drawCompositeQuad(unsigned int colorTexture, float width, float height);

        void releaseMeshGpu(std::uint64_t meshId);
        void releaseTextureGpu(std::uint64_t textureId);
        void destroyGlResources();

    private:
        Renderer3D() = default;
        ~Renderer3D();

        Renderer3D(Renderer3D const&) = delete;
        Renderer3D& operator=(Renderer3D const&) = delete;

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

        bool ensureLambertProgram();
        bool ensureLambertInstProgram();
        bool ensureBlitProgram();
        bool ensureBlitGeometry();
        bool ensureInstanceVbo();
        bool ensureDebugLineProgram();
        bool ensureDebugLineVbo();
        void drawDebugOverlay(
            glm::mat4 const& projection, glm::mat4 const& view,
            std::map<int, DebugLine> const& debugLines, bool debugBounds,
            std::map<int, ViewportInstance> const& instances
        );
        GpuMesh* ensureGpuMesh(std::uint64_t meshId, MeshAsset const& meshAsset);
        unsigned int ensureGpuTexture(std::uint64_t textureId, TextureAsset const& textureAsset);

        void deleteGpuMesh(GpuMesh& mesh);
        void deleteGpuPrimitive(GpuPrimitive& primitive);

        unsigned int m_lambertProgram = 0;
        int m_lambertLocMvp = -1;
        int m_lambertLocNormalMat = -1;
        int m_lambertLocLightDir = -1;
        int m_lambertLocLightColor = -1;
        int m_lambertLocAmbient = -1;
        int m_lambertLocBaseColor = -1;
        int m_lambertLocTexture = -1;
        int m_lambertLocUseTexture = -1;
        int m_lambertLocAlphaCutoff = -1;
        int m_lambertLocTint = -1;

        unsigned int m_lambertInstProgram = 0;
        int m_lambertInstLocViewProj = -1;
        int m_lambertInstLocLightDir = -1;
        int m_lambertInstLocLightColor = -1;
        int m_lambertInstLocAmbient = -1;
        int m_lambertInstLocBaseColor = -1;
        int m_lambertInstLocTexture = -1;
        int m_lambertInstLocUseTexture = -1;
        int m_lambertInstLocAlphaCutoff = -1;

        unsigned int m_instanceVbo = 0;

        unsigned int m_blitProgram = 0;
        int m_blitLocMvp = -1;
        int m_blitLocTexture = -1;
        unsigned int m_blitVbo = 0;

        unsigned int m_debugLineProgram = 0;
        int m_debugLineLocMvp = -1;
        int m_debugLineLocColor = -1;
        unsigned int m_debugLineVbo = 0;

        std::unordered_map<std::uint64_t, GpuMesh> m_gpuMeshes;
        std::unordered_map<std::uint64_t, unsigned int> m_gpuTextures;
    };

} // namespace luax::render3d
