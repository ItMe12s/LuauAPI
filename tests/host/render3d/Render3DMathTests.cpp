#include "render3d/types/Transform3D.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
    using namespace luax::render3d;
    using Catch::Approx;

    bool vec3Near(glm::vec3 const& a, glm::vec3 const& b, float epsilon = 1e-4f) {
        return glm::length(a - b) <= epsilon;
    }

    bool quatNear(glm::quat const& a, glm::quat const& b, float epsilon = 1e-4f) {
        glm::quat const lhs = glm::normalize(a);
        glm::quat const rhs = glm::normalize(b);
        return glm::abs(glm::dot(lhs, rhs)) >= 1.0f - epsilon;
    }

    bool transformNear(Transform const& a, Transform const& b, float epsilon = 1e-4f) {
        return vec3Near(a.position, b.position, epsilon) && quatNear(a.rotation, b.rotation, epsilon);
    }
} // namespace

TEST_CASE("Transform identity compose leaves transform unchanged") {
    Transform t{
        glm::vec3(2.0f, -1.0f, 0.5f),
        glm::normalize(glm::quat(glm::vec3(0.2f, 1.1f, -0.4f))),
    };

    REQUIRE(transformNear(Transform::identity() * t, t));
    REQUIRE(transformNear(t * Transform::identity(), t));
}

TEST_CASE("Transform inverse undoes composition") {
    Transform a{
        glm::vec3(1.0f, 0.0f, -3.0f),
        glm::angleAxis(0.7f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f))),
    };
    Transform b{
        glm::vec3(-2.0f, 4.0f, 1.0f),
        glm::angleAxis(-1.2f, glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f))),
    };

    Transform composed = a * b;
    REQUIRE(transformNear(composed * composed.inverse(), Transform::identity()));
    REQUIRE(transformNear(composed.inverse() * composed, Transform::identity()));
    REQUIRE(transformNear(a.inverse().inverse(), a));
}

TEST_CASE("Transform compose is associative") {
    Transform a = Transform::fromEuler(0.3f, -0.8f, 0.1f);
    a.position = glm::vec3(1.0f, 2.0f, 3.0f);
    Transform b = Transform::fromAxisAngle(glm::vec3(0.0f, 1.0f, 0.0f), 1.1f);
    b.position = glm::vec3(-1.0f, 0.5f, 2.0f);
    Transform c = Transform::fromLookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    REQUIRE(transformNear((a * b) * c, a * (b * c)));
}

TEST_CASE("Transform fromLookAt aims negative lookVector at target") {
    glm::vec3 const eye{0.0f, 2.0f, 6.0f};
    glm::vec3 const target{1.0f, -1.0f, 0.0f};
    Transform cam = Transform::fromLookAt(eye, target);

    REQUIRE(cam.position.x == Approx(eye.x));
    REQUIRE(cam.position.y == Approx(eye.y));
    REQUIRE(cam.position.z == Approx(eye.z));

    glm::vec3 const expected = glm::normalize(target - eye);
    REQUIRE(glm::dot(cam.lookVector(), expected) == Approx(1.0f).margin(1e-4f));
}

TEST_CASE("Transform basis vectors stay orthonormal") {
    Transform t = Transform::fromEuler(0.4f, 1.2f, -0.6f);
    glm::vec3 const right = t.rightVector();
    glm::vec3 const up = t.upVector();
    glm::vec3 const look = t.lookVector();

    REQUIRE(glm::length(right) == Approx(1.0f).margin(1e-4f));
    REQUIRE(glm::length(up) == Approx(1.0f).margin(1e-4f));
    REQUIRE(glm::length(look) == Approx(1.0f).margin(1e-4f));
    REQUIRE(glm::dot(right, up) == Approx(0.0f).margin(1e-4f));
    REQUIRE(glm::dot(right, look) == Approx(0.0f).margin(1e-4f));
    REQUIRE(glm::dot(up, look) == Approx(0.0f).margin(1e-4f));
}

TEST_CASE("Transform lerp hits endpoints and midpoint") {
    Transform start{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
    };
    Transform goal = Transform::fromEuler(0.0f, 1.5707963f, 0.0f);
    goal.position = glm::vec3(4.0f, -2.0f, 1.0f);

    REQUIRE(transformNear(start.lerp(goal, 0.0f), start));
    REQUIRE(transformNear(start.lerp(goal, 1.0f), goal));

    Transform mid = start.lerp(goal, 0.5f);
    REQUIRE(mid.position.x == Approx(2.0f).margin(1e-4f));
    REQUIRE(mid.position.y == Approx(-1.0f).margin(1e-4f));
    REQUIRE(mid.position.z == Approx(0.5f).margin(1e-4f));
}

TEST_CASE("Transform eulerAngles round-trips fromEuler") {
    float const pitch = 0.3f;
    float const yaw = -0.8f;
    float const roll = 0.1f;
    Transform const t = Transform::fromEuler(pitch, yaw, roll);
    glm::vec3 const angles = t.eulerAngles();

    REQUIRE(angles.x == Approx(pitch).margin(1e-4f));
    REQUIRE(angles.y == Approx(yaw).margin(1e-4f));
    REQUIRE(angles.z == Approx(roll).margin(1e-4f));
}

TEST_CASE("Transform withPosition preserves rotation") {
    Transform t = Transform::fromEuler(0.4f, 1.2f, -0.6f);
    t.position = glm::vec3(1.0f, 2.0f, 3.0f);
    glm::vec3 const newPos{5.0f, -1.0f, 0.25f};
    Transform const moved = t.withPosition(newPos);

    REQUIRE(vec3Near(moved.position, newPos));
    REQUIRE(quatNear(moved.rotation, t.rotation));
    REQUIRE(vec3Near(moved.rightVector(), t.rightVector()));
    REQUIRE(vec3Near(moved.upVector(), t.upVector()));
    REQUIRE(vec3Near(moved.lookVector(), t.lookVector()));
}

TEST_CASE("Transform withRotationOf preserves position") {
    Transform const t =
        Transform::fromLookAt(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    Transform const rot = Transform::fromEuler(0.5f, -1.0f, 0.2f);
    Transform const updated = t.withRotationOf(rot);

    REQUIRE(vec3Near(updated.position, t.position));
    REQUIRE(quatNear(updated.rotation, rot.rotation));
}

TEST_CASE("Transform toMat4 matches compose with point") {
    Transform parent = Transform::fromEuler(0.2f, 0.5f, -0.3f);
    parent.position = glm::vec3(3.0f, 1.0f, -2.0f);
    Transform child = Transform::fromAxisAngle(glm::vec3(1.0f, 0.0f, 0.0f), 0.9f);
    child.position = glm::vec3(0.0f, 2.0f, 0.0f);

    glm::vec4 const localPoint{1.0f, 0.0f, 0.0f, 1.0f};
    glm::vec4 const fromCompose = (parent * child).toMat4() * localPoint;
    glm::vec4 const fromMatrices = parent.toMat4() * child.toMat4() * localPoint;

    REQUIRE(fromCompose.x == Approx(fromMatrices.x).margin(1e-4f));
    REQUIRE(fromCompose.y == Approx(fromMatrices.y).margin(1e-4f));
    REQUIRE(fromCompose.z == Approx(fromMatrices.z).margin(1e-4f));
}
