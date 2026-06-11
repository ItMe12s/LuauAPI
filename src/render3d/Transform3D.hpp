#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace luax::render3d {

    namespace detail {
        inline glm::vec3 rotateVec(glm::quat const& rotation, glm::vec3 const& vector) {
            return glm::mat3_cast(rotation) * vector;
        }
    } // namespace detail

    struct Transform {
        glm::vec3 position{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

        Transform() = default;

        Transform(glm::vec3 pos, glm::quat rot) : position(pos), rotation(rot) {}

        static Transform identity() {
            return Transform{};
        }

        static Transform fromLookAt(
            glm::vec3 const& position, glm::vec3 const& target,
            glm::vec3 const& up = glm::vec3(0.0f, 1.0f, 0.0f)
        ) {
            glm::vec3 direction = target - position;
            float const len = glm::length(direction);
            glm::quat rot{1.0f, 0.0f, 0.0f, 0.0f};
            if (len > 1e-6f) {
                direction /= len;
                rot = glm::quatLookAtRH(direction, up);
            }
            return Transform{position, rot};
        }

        static Transform fromAxisAngle(glm::vec3 const& axis, float angleRadians) {
            return Transform{glm::vec3(0.0f), glm::angleAxis(angleRadians, axis)};
        }

        static Transform fromEuler(float pitchRadians, float yawRadians, float rollRadians) {
            return Transform{
                glm::vec3(0.0f),
                glm::quat(glm::vec3(pitchRadians, yawRadians, rollRadians)),
            };
        }

        Transform operator*(Transform const& other) const {
            return Transform{
                position + detail::rotateVec(rotation, other.position),
                glm::normalize(rotation * other.rotation),
            };
        }

        Transform inverse() const {
            glm::quat const invRot = glm::conjugate(rotation);
            return Transform{detail::rotateVec(invRot, -position), invRot};
        }

        Transform lerp(Transform const& goal, float alpha) const {
            return Transform{
                glm::mix(position, goal.position, alpha),
                glm::slerp(rotation, goal.rotation, alpha),
            };
        }

        glm::mat4 toMat4() const {
            return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
        }

        glm::vec3 rightVector() const {
            return detail::rotateVec(rotation, glm::vec3(1.0f, 0.0f, 0.0f));
        }

        glm::vec3 upVector() const {
            return detail::rotateVec(rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::vec3 lookVector() const {
            return detail::rotateVec(rotation, glm::vec3(0.0f, 0.0f, -1.0f));
        }
    };

} // namespace luax::render3d
