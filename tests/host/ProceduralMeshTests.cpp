#include "render3d/assets/MeshAsset.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

namespace {
    using Catch::Approx;
    using namespace luax::render3d;

    bool vec3Near(glm::vec3 const& a, glm::vec3 const& b, float epsilon = 1e-4f) {
        return glm::length(a - b) <= epsilon;
    }

    std::shared_ptr<MeshAsset> requireMesh(LoadResult<std::shared_ptr<MeshAsset>> result) {
        REQUIRE(result.isOk());
        return result.unwrap();
    }
} // namespace

TEST_CASE("MeshAsset fromBuffers computes flat normals for one triangle") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2};

    auto mesh = requireMesh(MeshAsset::fromBuffers(positions, {}, {}, indices));
    REQUIRE(mesh->primitiveCount() == 1);

    auto const& primitive = mesh->primitives().front();
    REQUIRE(primitive.normals.size() == 3);
    for (auto const& normal : primitive.normals) {
        REQUIRE(vec3Near(normal, glm::vec3(0.0f, 0.0f, 1.0f)));
    }
}

TEST_CASE("MeshAsset fromBuffers computes bounding box") {
    std::vector<glm::vec3> positions{
        glm::vec3(-2.0f, 0.5f, 1.0f),
        glm::vec3(3.0f, -1.0f, 4.0f),
        glm::vec3(0.0f, 2.0f, -3.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2};

    auto mesh = requireMesh(MeshAsset::fromBuffers(positions, {}, {}, indices));
    auto const& bounds = mesh->boundingBox();

    REQUIRE_FALSE(bounds.empty);
    REQUIRE(bounds.min.x == Approx(-2.0f));
    REQUIRE(bounds.min.y == Approx(-1.0f));
    REQUIRE(bounds.min.z == Approx(-3.0f));
    REQUIRE(bounds.max.x == Approx(3.0f));
    REQUIRE(bounds.max.y == Approx(2.0f));
    REQUIRE(bounds.max.z == Approx(4.0f));
    REQUIRE(mesh->vertexCount() == 3);
}

TEST_CASE("MeshAsset fromBuffers rejects out-of-range indices") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 3};

    auto result = MeshAsset::fromBuffers(positions, {}, {}, indices);
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr() == "index out of range");
}

TEST_CASE("MeshAsset fromBuffers rejects unconverted one-based indices") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{1, 2, 3};

    auto result = MeshAsset::fromBuffers(positions, {}, {}, indices);
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr() == "index out of range");
}

TEST_CASE("MeshAsset fromBuffers accepts indices after one-based conversion") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };

    std::vector<std::uint32_t> indices;
    for (int luaIndex : {1, 2, 3}) {
        indices.push_back(static_cast<std::uint32_t>(luaIndex - 1));
    }

    auto mesh = requireMesh(MeshAsset::fromBuffers(positions, {}, {}, indices));
    REQUIRE(mesh->vertexCount() == 3);
    REQUIRE(mesh->primitiveCount() == 1);
}
