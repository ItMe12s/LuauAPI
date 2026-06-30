#pragma once

#include "render3d/types/Material.hpp"
#include "render3d/types/Transform3D.hpp"

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <map>
#include <memory>

namespace luax::render3d {

    class MeshAsset;

    inline constexpr glm::vec3 kDefaultLightDirection{0.35f, 0.85f, 0.4f};

    struct Camera3D {
        Transform transform{};
        float fovYDegrees = 70.0f;
        float zNear = 0.1f;
        float zFar = 100.0f;
    };

    struct RenderSettings {
        glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 lightDirection{kDefaultLightDirection};
        glm::vec3 lightColor{1.0f, 1.0f, 1.0f};
        float lightIntensity = 1.0f;
        float ambient = 0.15f;
    };

    struct ViewportInstance {
        std::uint64_t meshId = 0;
        std::shared_ptr<MeshAsset> mesh{};
        Transform transform{};
        glm::vec3 color{1.0f, 1.0f, 1.0f};
        std::shared_ptr<Material> materialOverride{};
        std::map<int, std::shared_ptr<Material>> primitiveOverrides{};
    };

    struct DebugLine {
        glm::vec3 from{};
        glm::vec3 to{};
        glm::vec3 color{1.0f, 1.0f, 1.0f};
    };

} // namespace luax::render3d
