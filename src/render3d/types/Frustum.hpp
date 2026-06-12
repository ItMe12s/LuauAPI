#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

namespace luax::render3d {

    struct Frustum {
        glm::vec4 planes[6]{};

        static Frustum fromViewProj(glm::mat4 const& viewProj) {
            Frustum frustum{};

            auto const row = [&](int index) {
                return glm::vec4(
                    viewProj[0][index], viewProj[1][index], viewProj[2][index], viewProj[3][index]
                );
            };

            frustum.planes[0] = row(3) + row(0);
            frustum.planes[1] = row(3) - row(0);
            frustum.planes[2] = row(3) + row(1);
            frustum.planes[3] = row(3) - row(1);
            frustum.planes[4] = row(3) + row(2);
            frustum.planes[5] = row(3) - row(2);

            for (glm::vec4& plane : frustum.planes) {
                float const length = glm::length(glm::vec3(plane));
                if (length > 1e-6f) {
                    plane /= length;
                }
            }

            return frustum;
        }

        bool intersectsSphere(glm::vec3 center, float radius) const {
            for (glm::vec4 const& plane : planes) {
                float const distance = glm::dot(glm::vec3(plane), center) + plane.w;
                if (distance < -radius) {
                    return false;
                }
            }
            return true;
        }
    };

} // namespace luax::render3d
