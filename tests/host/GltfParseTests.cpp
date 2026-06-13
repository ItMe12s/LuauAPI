#include "render3d/assets/MeshAsset.hpp"

#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace {
    using Catch::Approx;
    using namespace luax::render3d;

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

    std::shared_ptr<MeshAsset> loadGltfJson(std::string const& gltfJson) {
        std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
        auto const sandbox = repoRoot();
        auto result = MeshAsset::loadFromBytes(
            std::span<std::uint8_t const>(bytes.data(), bytes.size()),
            sandbox / "fixture.gltf",
            sandbox
        );
        REQUIRE(result.isOk());
        return result.unwrap();
    }

    void requireGltfError(std::string const& gltfJson, std::string const& messagePart) {
        std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
        auto const sandbox = repoRoot();
        auto result = MeshAsset::loadFromBytes(
            std::span<std::uint8_t const>(bytes.data(), bytes.size()),
            sandbox / "fixture.gltf",
            sandbox
        );
        REQUIRE(result.isErr());
        REQUIRE(result.unwrapErr().find(messagePart) != std::string::npos);
    }

    std::shared_ptr<MeshAsset> loadMaterialFlagsFixture(char const* materialsJson) {
        std::string gltfJson = std::string(R"({"asset": {"version": "2.0"}, "materials": [)") +
            materialsJson + kMinimalTriangleGltfSuffix;
        return loadGltfJson(gltfJson);
    }

    std::string minimalTriangleGltfWithPrefix(std::string const& prefix) {
        return prefix + kMinimalTriangleGltfSuffix;
    }
} // namespace

TEST_CASE("MeshAsset parses resources/test_donut.glb") {
    auto const path = repoRoot() / "resources" / "test_donut.glb";
    INFO(path);

    REQUIRE(std::filesystem::exists(path));

    auto result = MeshAsset::loadFromFile(path);
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

    auto result = MeshAsset::loadFromFile(path);
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

TEST_CASE("MeshAsset loadFromBytes rejects empty bytes") {
    std::vector<std::uint8_t> const empty;
    auto const sandbox = repoRoot();
    auto result = MeshAsset::loadFromBytes(
        std::span<std::uint8_t const>(empty.data(), empty.size()),
        sandbox / "empty.gltf",
        sandbox
    );
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr() == "glTF data is empty");
}

TEST_CASE("MeshAsset loadFromBytes rejects invalid JSON") {
    std::string const invalidJson = R"({"asset": {"version": "2.0"}, "broken)";
    requireGltfError(invalidJson, "invalid json");
}

TEST_CASE("MeshAsset loadFromFile rejects missing path") {
    auto const path = repoRoot() / "resources" / "missing_mesh_fixture_xyz.glb";
    INFO(path);
    REQUIRE_FALSE(std::filesystem::exists(path));

    auto result = MeshAsset::loadFromFile(path);
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr().find("glTF file cannot be read") != std::string::npos);
}

TEST_CASE("MeshAsset loadFromBytes rejects glTF with no mesh primitives") {
    requireGltfError(R"({"asset": {"version": "2.0"}, "nodes": []})", "no mesh primitives");
}

TEST_CASE("MeshAsset loadFromBytes parses test_donut.glb bytes") {
    auto const path = repoRoot() / "resources" / "test_donut.glb";
    INFO(path);
    REQUIRE(std::filesystem::exists(path));

    std::ifstream input(path, std::ios::binary);
    REQUIRE(input.good());
    std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>()
    );
    REQUIRE_FALSE(bytes.empty());

    auto result = MeshAsset::loadFromBytes(bytes, path, path.parent_path());
    REQUIRE(result.isOk());
    REQUIRE(result.unwrap()->vertexCount() > 0);
}

TEST_CASE("MeshAsset loadFromBytes parses baseColorFactor into material color") {
    auto const mesh = loadMaterialFlagsFixture(
        R"({"pbrMetallicRoughness": {"baseColorFactor": [0.25, 0.5, 0.75, 0.8]}})"
    );

    REQUIRE(mesh->materialCount() == 1);
    auto const& material = mesh->materials().front();
    REQUIRE(material.baseColorFactor.x == Approx(0.25f));
    REQUIRE(material.baseColorFactor.y == Approx(0.5f));
    REQUIRE(material.baseColorFactor.z == Approx(0.75f));
    REQUIRE(material.baseColorFactor.w == Approx(0.8f));
}

TEST_CASE("MeshAsset loadFromBytes rejects textured primitive without TEXCOORD_0") {
    char const* const k1x1PngBase64 =
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";

    std::string gltfJson = minimalTriangleGltfWithPrefix(
        std::string("{\n  \"asset\": {\"version\": \"2.0\"},\n  \"images\": [{\"uri\": \"data:image/png;base64,") +
        k1x1PngBase64 +
        "\"}],\n  \"textures\": [{\"source\": 0}],\n  \"materials\": "
        "[{\"pbrMetallicRoughness\": {\"baseColorTexture\": {\"index\": 0}}}"
    );

    requireGltfError(gltfJson, "textures require TEXCOORD_0");
}

TEST_CASE("MeshAsset loadFromBytes rejects Draco compressed primitives") {
    requireGltfError(R"({
  "asset": {"version": "2.0"},
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
      "extensions": {
        "KHR_draco_mesh_compression": {
          "bufferView": 0,
          "attributes": {"POSITION": 0}
        }
      }
    }]
  }],
  "nodes": [{"mesh": 0}],
  "scenes": [{"nodes": [0]}],
  "scene": 0
})", "Draco compressed primitives are not supported");
}

TEST_CASE("MeshAsset loadFromBytes rejects meshopt compressed accessors") {
    requireGltfError(R"({
  "asset": {"version": "2.0"},
  "buffers": [{
    "byteLength": 42,
    "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA"
  }],
  "bufferViews": [{
    "buffer": 0,
    "byteOffset": 0,
    "byteLength": 36,
    "extensions": {
      "EXT_meshopt_compression": {
        "buffer": 0,
        "byteOffset": 0,
        "byteLength": 36,
        "byteStride": 12,
        "count": 3,
        "mode": "ATTRIBUTES"
      }
    }
  }, {
    "buffer": 0,
    "byteOffset": 36,
    "byteLength": 6
  }],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}
  ],
  "meshes": [{
    "primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]
  }],
  "nodes": [{"mesh": 0}],
  "scenes": [{"nodes": [0]}],
  "scene": 0
})", "meshopt-compressed accessors are not supported");
}

TEST_CASE("MeshAsset loadFromBytes rejects sparse accessors") {
    requireGltfError(R"({
  "asset": {"version": "2.0"},
  "buffers": [{
    "byteLength": 56,
    "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAIA/AAAAAAEAAgA="
  }],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 2},
    {"buffer": 0, "byteOffset": 38, "byteLength": 12},
    {"buffer": 0, "byteOffset": 50, "byteLength": 6}
  ],
  "accessors": [{
    "bufferView": 0,
    "componentType": 5126,
    "count": 3,
    "type": "VEC3",
    "sparse": {
      "count": 1,
      "indices": {"bufferView": 1, "componentType": 5123, "byteOffset": 0},
      "values": {"bufferView": 2, "byteOffset": 0}
    }
  }, {"bufferView": 3, "componentType": 5123, "count": 3, "type": "SCALAR"}],
  "meshes": [{
    "primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]
  }],
  "nodes": [{"mesh": 0}],
  "scenes": [{"nodes": [0]}],
  "scene": 0
})", "sparse accessors are not supported");
}

TEST_CASE("MeshAsset loadFromBytes rejects external buffer outside sandbox") {
    auto const base = std::filesystem::temp_directory_path() /
        ("luauapi_gltf_sandbox_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto const sandbox = base / "sandbox";
    auto const outsideBuffer = base / "outside.bin";
    auto const gltfPath = sandbox / "model.gltf";

    REQUIRE(std::filesystem::create_directories(sandbox));

    {
        std::ofstream outside(outsideBuffer, std::ios::binary);
        REQUIRE(outside.good());
        std::array<std::uint8_t, 42> const bufferBytes{
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 128, 63, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0,
        };
        outside.write(reinterpret_cast<char const*>(bufferBytes.data()), bufferBytes.size());
        REQUIRE(outside.good());
    }

    std::string const gltfJson = R"({
  "asset": {"version": "2.0"},
  "buffers": [{"byteLength": 42, "uri": "../outside.bin"}],
  "bufferViews": [
    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
    {"buffer": 0, "byteOffset": 36, "byteLength": 6}
  ],
  "accessors": [
    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
    {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}
  ],
  "meshes": [{
    "primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]
  }],
  "nodes": [{"mesh": 0}],
  "scenes": [{"nodes": [0]}],
  "scene": 0
})";

    {
        std::ofstream gltf(gltfPath);
        REQUIRE(gltf.good());
        gltf << gltfJson;
        REQUIRE(gltf.good());
    }

    std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
    auto result = MeshAsset::loadFromBytes(bytes, gltfPath, sandbox);
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr().find("escapes sandbox root") != std::string::npos);

    std::filesystem::remove_all(base);
}
