#pragma once

#include "render3d/gpu/Renderer3DPrograms.hpp"
#include "render3d/types/SceneTypes.hpp"

#include <glm/mat4x4.hpp>
#include <map>

namespace luax::render3d {

    void drawDebugOverlay(
        Renderer3DPrograms& programs, glm::mat4 const& projection, glm::mat4 const& view,
        std::map<int, DebugLine> const& debugLines, bool debugBounds,
        std::map<int, ViewportInstance> const& instances
    );

} // namespace luax::render3d
