#pragma once

#include "render3d/gpu/Renderer3DMeshCache.hpp"
#include "render3d/gpu/Renderer3DPrograms.hpp"
#include "render3d/types/SceneTypes.hpp"

#include <cstdint>
#include <map>
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

        void releaseMeshGpu(std::uint64_t meshId);
        void releaseTextureGpu(std::uint64_t textureId);
        void destroyGlResources();

    private:
        Renderer3D() = default;
        ~Renderer3D();

        Renderer3D(Renderer3D const&) = delete;
        Renderer3D& operator=(Renderer3D const&) = delete;

        void syncContextGen();

        Renderer3DPrograms m_programs;
        Renderer3DMeshCache m_meshCache;
        unsigned m_gen = 0;
    };

} // namespace luax::render3d
