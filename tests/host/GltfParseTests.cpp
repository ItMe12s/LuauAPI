#include "render3d/MeshAsset.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

namespace {
    std::filesystem::path repoRoot() {
        return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    }
} // namespace

TEST_CASE("MeshAsset parses resources/test_donut.glb") {
    auto const path = repoRoot() / "resources" / "test_donut.glb";
    INFO(path);

    REQUIRE(std::filesystem::exists(path));

    auto result = luax::render3d::MeshAsset::loadFromFile(path);
    REQUIRE(result.isOk());

    auto const mesh = result.unwrap();
    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->vertexCount() > 0);
    REQUIRE(mesh->primitiveCount() > 0);

    auto const& bounds = mesh->boundingBox();
    REQUIRE_FALSE(bounds.empty);
    REQUIRE(bounds.min.x <= bounds.max.x);
    REQUIRE(bounds.min.y <= bounds.max.y);
    REQUIRE(bounds.min.z <= bounds.max.z);

    auto const& primitives = mesh->primitives();
    REQUIRE(primitives.size() == mesh->primitiveCount());
    for (auto const& primitive : primitives) {
        REQUIRE_FALSE(primitive.positions.empty());
        REQUIRE(primitive.normals.size() == primitive.positions.size());
        REQUIRE_FALSE(primitive.indices.empty());
        REQUIRE(primitive.indices.size() % 3 == 0);
    }

}

TEST_CASE("MeshAsset parses glTF materials and textures from test_donut.glb") {
    auto const path = repoRoot() / "resources" / "test_donut.glb";
    INFO(path);

    REQUIRE(std::filesystem::exists(path));

    auto result = luax::render3d::MeshAsset::loadFromFile(path);
    REQUIRE(result.isOk());

    auto const mesh = result.unwrap();
    REQUIRE(mesh->materialCount() == 1);

    auto const& materials = mesh->materials();
    REQUIRE(materials.size() == 1);
    REQUIRE(materials[0].imageIndex == 0);

    auto const& images = mesh->images();
    REQUIRE(images.size() == 1);
    REQUIRE(images[0].width > 0);
    REQUIRE(images[0].height > 0);
    REQUIRE(images[0].rgba.size() ==
            static_cast<std::size_t>(images[0].width) * static_cast<std::size_t>(images[0].height) * 4);

    bool hasNonZeroPixel = false;
    for (auto const byte : images[0].rgba) {
        if (byte != 0) {
            hasNonZeroPixel = true;
            break;
        }
    }
    REQUIRE(hasNonZeroPixel);

    auto const& primitives = mesh->primitives();
    REQUIRE(primitives.size() >= 1);
    REQUIRE(primitives[0].materialIndex == 0);
    REQUIRE(primitives[0].texcoords.size() == primitives[0].positions.size());
    REQUIRE_FALSE(primitives[0].texcoords.empty());
}
