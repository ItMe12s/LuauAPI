#include "render3d/types/Frustum.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    using namespace luax::render3d;
    using Catch::Approx;

    bool planeNormalized(glm::vec4 const& plane, float epsilon = 1e-4f) {
        return glm::length(glm::vec3(plane)) == Approx(1.0f).margin(epsilon);
    }
} // namespace

TEST_CASE("Frustum fromViewProj identity produces normalized planes") {
    Frustum const frustum = Frustum::fromViewProj(glm::mat4(1.0f));

    for (glm::vec4 const& plane : frustum.planes) {
        REQUIRE(planeNormalized(plane));
    }

    REQUIRE(frustum.intersectsSphere(glm::vec3(0.0f, 0.0f, -0.5f), 0.1f));
}

TEST_CASE("Frustum ortho culling accepts sphere inside volume") {
    glm::mat4 const viewProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);
    Frustum const frustum = Frustum::fromViewProj(viewProj);

    REQUIRE(frustum.intersectsSphere(glm::vec3(0.0f, 0.0f, -5.0f), 0.25f));
}

TEST_CASE("Frustum ortho culling rejects sphere outside each plane") {
    glm::mat4 const viewProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);
    Frustum const frustum = Frustum::fromViewProj(viewProj);
    float const radius = 0.1f;
    float const depth = -5.0f;

    REQUIRE_FALSE(frustum.intersectsSphere(glm::vec3(-5.0f, 0.0f, depth), radius));
    REQUIRE_FALSE(frustum.intersectsSphere(glm::vec3(5.0f, 0.0f, depth), radius));
    REQUIRE_FALSE(frustum.intersectsSphere(glm::vec3(0.0f, -5.0f, depth), radius));
    REQUIRE_FALSE(frustum.intersectsSphere(glm::vec3(0.0f, 5.0f, depth), radius));
    REQUIRE_FALSE(frustum.intersectsSphere(glm::vec3(0.0f, 0.0f, 0.0f), radius));
    REQUIRE_FALSE(frustum.intersectsSphere(glm::vec3(0.0f, 0.0f, -20.0f), radius));
}

TEST_CASE("Frustum ortho culling accepts sphere straddling a plane") {
    glm::mat4 const viewProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);
    Frustum const frustum = Frustum::fromViewProj(viewProj);

    REQUIRE(frustum.intersectsSphere(glm::vec3(0.95f, 0.0f, -5.0f), 0.2f));
    REQUIRE(frustum.intersectsSphere(glm::vec3(-0.95f, 0.0f, -5.0f), 0.2f));
}

TEST_CASE("Frustum manual plane rejects sphere fully outside one side") {
    Frustum frustum{};
    frustum.planes[0] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    frustum.planes[1] = glm::vec4(-1.0f, 0.0f, 0.0f, 1.0f);
    frustum.planes[2] = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    frustum.planes[3] = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
    frustum.planes[4] = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    frustum.planes[5] = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

    REQUIRE(frustum.intersectsSphere(glm::vec3(0.0f, 0.0f, 0.0f), 0.25f));
    REQUIRE_FALSE(frustum.intersectsSphere(glm::vec3(-3.0f, 0.0f, 0.0f), 0.25f));
}
