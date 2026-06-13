#pragma once

#include "render3d/gpu/Renderer3DMeshCache.hpp"
#include "render3d/gpu/Renderer3DPrograms.hpp"
#include "render3d/types/SceneTypes.hpp"

#include <cstdint>

namespace luax::render3d {

    void runRenderer3DScenePass(
        Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache, int pixelWidth,
        int pixelHeight, Camera3D const& camera, std::map<int, ViewportInstance> const& instances,
        RenderSettings const& settings, int selfColorTexture
    );

} // namespace luax::render3d
