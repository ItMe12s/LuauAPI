#pragma once

#include "render3d/CCViewportFrameNode.hpp"

#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

namespace luax::render3d {

    class Renderer3D {
    public:
        static Renderer3D& instance();

        void renderToFramebuffer(
            unsigned int fbo, int pixelWidth, int pixelHeight, Camera3D const& camera,
            std::map<int, ViewportInstance> const& instances
        );

        void drawCompositeQuad(unsigned int colorTexture, float width, float height);

        void releaseMeshGpu(std::uint64_t meshId);
        void destroyGlResources();

    private:
        Renderer3D() = default;
        ~Renderer3D();

        Renderer3D(Renderer3D const&) = delete;
        Renderer3D& operator=(Renderer3D const&) = delete;

        struct GpuPrimitive {
            unsigned int vbo = 0;
            unsigned int ibo = 0;
            unsigned int indexCount = 0;
        };

        struct GpuMesh {
            std::vector<GpuPrimitive> primitives;
        };

        bool ensureLambertProgram();
        bool ensureBlitProgram();
        bool ensureBlitGeometry();
        GpuMesh* ensureGpuMesh(std::uint64_t meshId);

        void deleteGpuMesh(GpuMesh& mesh);
        void deleteGpuPrimitive(GpuPrimitive& primitive);

        unsigned int m_lambertProgram = 0;
        int m_lambertLocMvp = -1;
        int m_lambertLocNormalMat = -1;
        int m_lambertLocLightDir = -1;
        int m_lambertLocColor = -1;

        unsigned int m_blitProgram = 0;
        int m_blitLocMvp = -1;
        int m_blitLocTexture = -1;
        unsigned int m_blitVbo = 0;

        std::unordered_map<std::uint64_t, GpuMesh> m_gpuMeshes;
    };

} // namespace luax::render3d
