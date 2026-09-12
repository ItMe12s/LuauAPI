#pragma once

#include "render3d/gpu/Renderer3D.hpp"

#include <glm/mat4x4.hpp>
#include <unordered_map>

namespace luax::render3d {

    void destroyRenderer3DGlResources(Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache);
    void ensureRenderer3DShutdownHook();
    void runRenderer3DScenePass(
        Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache, int pixelWidth, int pixelHeight,
        Camera3D const& camera, std::unordered_map<int, ViewportInstance> const& instances,
        RenderSettings const& settings, int selfColorTexture
    );
    void drawDebugOverlay(
        Renderer3DPrograms& programs, glm::mat4 const& projection, glm::mat4 const& view,
        std::unordered_map<int, DebugLine> const& debugLines, bool debugBounds,
        std::unordered_map<int, ViewportInstance> const& instances
    );

} // namespace luax::render3d
