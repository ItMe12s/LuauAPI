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

    std::shared_ptr<MeshAsset> requireMesh(std::expected<std::shared_ptr<MeshAsset>, std::string> result) {
        REQUIRE(result.has_value());
        return std::move(result).value();
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
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "index out of range");
}

TEST_CASE("MeshAsset fromBuffers rejects unconverted one-based indices") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{1, 2, 3};

    auto result = MeshAsset::fromBuffers(positions, {}, {}, indices);
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "index out of range");
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

TEST_CASE("MeshAsset fromBuffers rejects empty positions") {
    std::vector<std::uint32_t> indices{0, 1, 2};
    auto result = MeshAsset::fromBuffers({}, {}, {}, indices);
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "positions are empty");
}

TEST_CASE("MeshAsset fromBuffers rejects index count not divisible by three") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2, 3};

    auto result = MeshAsset::fromBuffers(positions, {}, {}, indices);
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "indices must contain a multiple of three triangle indices");
}

TEST_CASE("MeshAsset fromBuffers rejects mismatched normals length") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<glm::vec3> normals{
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2};

    auto result = MeshAsset::fromBuffers(positions, normals, {}, indices);
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "normals length must match positions or be omitted");
}

TEST_CASE("MeshAsset fromBuffers rejects mismatched UV length") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<glm::vec2> uvs{
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2};

    auto result = MeshAsset::fromBuffers(positions, {}, uvs, indices);
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "uvs length must match positions or be omitted");
}

TEST_CASE("MeshAsset fromBuffers preserves supplied normals") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<glm::vec3> normals{
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2};

    auto mesh = requireMesh(MeshAsset::fromBuffers(positions, normals, {}, indices));
    auto const& stored = mesh->primitives().front().normals;
    REQUIRE(stored.size() == 3);
    REQUIRE(vec3Near(stored[0], normals[0]));
    REQUIRE(vec3Near(stored[1], normals[1]));
    REQUIRE(vec3Near(stored[2], normals[2]));
}

TEST_CASE("MeshAsset fromBuffers preserves supplied UVs") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<glm::vec2> uvs{
        glm::vec2(0.25f, 0.5f),
        glm::vec2(0.75f, 0.125f),
        glm::vec2(1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2};

    auto mesh = requireMesh(MeshAsset::fromBuffers(positions, {}, uvs, indices));
    auto const& stored = mesh->primitives().front().texcoords;
    REQUIRE(stored.size() == 3);
    REQUIRE(stored[0].x == Approx(uvs[0].x));
    REQUIRE(stored[0].y == Approx(uvs[0].y));
    REQUIRE(stored[1].x == Approx(uvs[1].x));
    REQUIRE(stored[1].y == Approx(uvs[1].y));
    REQUIRE(stored[2].x == Approx(uvs[2].x));
    REQUIRE(stored[2].y == Approx(uvs[2].y));
}

TEST_CASE("MeshAsset fromBuffers rejects vertex count above limit") {
    std::vector<glm::vec3> positions(kMaxProceduralMeshVertices + 1, glm::vec3(0.0f));
    std::vector<std::uint32_t> indices{0, 1, 2};

    auto result = MeshAsset::fromBuffers(positions, {}, {}, indices);
    REQUIRE(!result.has_value());
    REQUIRE(result.error() == "positions exceed maximum vertex count");
}

TEST_CASE("MeshRegistry register get release round trip") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2};
    auto mesh = requireMesh(MeshAsset::fromBuffers(positions, {}, {}, indices));

    auto& registry = MeshRegistry::instance();
    auto const id = registry.registerMesh(mesh);
    REQUIRE(registry.get(id) == mesh);

    registry.release(id);
    REQUIRE(registry.get(id) == nullptr);
}

TEST_CASE("MeshRegistry get returns null for unknown id") {
    auto& registry = MeshRegistry::instance();
    REQUIRE(registry.get(0xFFFF'FFFF'FFFF'FFFFULL) == nullptr);
}

TEST_CASE("MeshRegistry ids are monotonic") {
    std::vector<glm::vec3> positions{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
    };
    std::vector<std::uint32_t> indices{0, 1, 2};
    auto meshA = requireMesh(MeshAsset::fromBuffers(positions, {}, {}, indices));
    auto meshB = requireMesh(MeshAsset::fromBuffers(positions, {}, {}, indices));

    auto& registry = MeshRegistry::instance();
    auto const idA = registry.registerMesh(meshA);
    auto const idB = registry.registerMesh(meshB);
    REQUIRE(idB > idA);

    registry.release(idA);
    registry.release(idB);
}
