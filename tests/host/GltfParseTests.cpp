#include "render3d/MeshAsset.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <span>
#include <vector>

namespace {
    using Catch::Approx;
    std::filesystem::path repoRoot() {
        return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    }

    char const kMinimalTriangleGltfSuffix[] = R"(],
  "buffers": [{
    "byteLength": 42,
    "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA"
  }],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 6}
  ],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}
  ],
  "meshes": [{
    "primitives": [{
      "attributes": {"POSITION": 0},
      "indices": 1,
      "material": 0
    }]
  }],
  "nodes": [{"mesh": 0}],
  "scenes": [{"nodes": [0]}],
  "scene": 0
})";

    std::shared_ptr<luax::render3d::MeshAsset> loadMaterialFlagsFixture(char const* materialsJson) {
        std::string gltfJson = std::string(R"({"asset": {"version": "2.0"}, "materials": [)") +
            materialsJson + kMinimalTriangleGltfSuffix;

        std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
        auto const sandbox = repoRoot();
        auto result = luax::render3d::MeshAsset::loadFromBytes(
            std::span<std::uint8_t const>(bytes.data(), bytes.size()),
            sandbox / "material_flags.gltf",
            sandbox
        );
        REQUIRE(result.isOk());
        return result.unwrap();
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

    REQUIRE(materials[0].alphaMode == 0);
    REQUIRE(materials[0].alphaCutoff == Approx(0.5f));
    REQUIRE(materials[0].doubleSided);
}

TEST_CASE("MeshAsset parses material alphaMode alphaCutoff doubleSided") {
    auto const mesh = loadMaterialFlagsFixture(
        R"({"alphaMode": "BLEND"}, {"alphaMode": "MASK", "alphaCutoff": 0.25}, {"doubleSided": true})"
    );

    REQUIRE(mesh->materialCount() == 3);

    auto const& materials = mesh->materials();
    REQUIRE(materials.size() == 3);

    REQUIRE(materials[0].alphaMode == 2);
    REQUIRE(materials[0].alphaCutoff == Approx(0.5f));
    REQUIRE_FALSE(materials[0].doubleSided);

    REQUIRE(materials[1].alphaMode == 1);
    REQUIRE(materials[1].alphaCutoff == Approx(0.25f));
    REQUIRE_FALSE(materials[1].doubleSided);

    REQUIRE(materials[2].alphaMode == 0);
    REQUIRE(materials[2].alphaCutoff == Approx(0.5f));
    REQUIRE(materials[2].doubleSided);
}
