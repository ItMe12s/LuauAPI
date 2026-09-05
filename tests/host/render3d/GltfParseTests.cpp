#include "core/Config.hpp"
#include "lua_test_helpers.hpp"
#include "render3d/assets/GltfIo.hpp"
#include "render3d/assets/MeshAsset.hpp"

#include <Geode/utils/base64.hpp>
#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cgltf.h>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
    using Catch::Approx;
    using namespace luax::render3d;

    std::filesystem::path repoRoot() {
        return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
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

    constexpr std::array<std::uint8_t, 42> kTriangleBufferBytes{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   128, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 63,  0,  0, 0, 0, 1, 0, 2, 0, 0, 0,
    };

    std::string externalTriangleGltf(std::string_view uri) {
        return std::string(R"({
    "asset": {"version": "2.0"},
    "buffers": [{"byteLength": 42, "uri": ")") +
            std::string(uri) + R"("}],
    "bufferViews": [
        {"buffer": 0, "byteOffset": 0, "byteLength": 36},
        {"buffer": 0, "byteOffset": 36, "byteLength": 6}
    ],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
        {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}
    ],
    "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
    "nodes": [{"mesh": 0}],
    "scenes": [{"nodes": [0]}],
    "scene": 0
})";
    }

    void writeSparseFile(std::filesystem::path const& path, std::size_t size) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        REQUIRE(output.good());
        output.seekp(static_cast<std::streamoff>(size - 1));
        output.put('\0');
        REQUIRE(output.good());
    }

    std::shared_ptr<MeshAsset> loadGltfJson(std::string const& gltfJson) {
        std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
        auto const sandbox = repoRoot();
        auto result = MeshAsset::loadFromBytes(
            std::span<std::uint8_t const>(bytes.data(), bytes.size()), sandbox / "fixture.gltf", sandbox
        );
        REQUIRE(result.isOk());
        return std::move(result).unwrap();
    }

    void requireGltfError(std::string const& gltfJson, std::string const& messagePart) {
        std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
        auto const sandbox = repoRoot();
        auto result = MeshAsset::loadFromBytes(
            std::span<std::uint8_t const>(bytes.data(), bytes.size()), sandbox / "fixture.gltf", sandbox
        );
        REQUIRE(result.isErr());
        REQUIRE(result.unwrapErr().find(messagePart) != std::string::npos);
    }

    std::shared_ptr<MeshAsset> loadMaterialFlagsFixture(char const* materialsJson) {
        std::string gltfJson = std::string(R"({"asset": {"version": "2.0"}, "materials": [)") +
            materialsJson + kMinimalTriangleGltfSuffix;
        return loadGltfJson(gltfJson);
    }

} // namespace

TEST_CASE("MeshAsset rejects glTF primitive above vertex limit before unpacking") {
    std::string const gltfJson = std::string(R"({
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
        {"bufferView": 0, "componentType": 5126, "count": 65536, "type": "VEC3"},
        {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}
    ],
    "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
    "nodes": [{"mesh": 0}],
    "scenes": [{"nodes": [0]}],
    "scene": 0
})");
    requireGltfError(gltfJson, "primitive exceeds maximum vertex count");
}

// NOTE: decode-backed cases (donut texture, TEXCOORD_0) live on device only.
// ImagePlus is not linked on host, decode stubs Err.

TEST_CASE("MeshAsset parses material alphaMode alphaCutoff doubleSided") {
    auto const mesh = loadMaterialFlagsFixture(
        R"({"alphaMode": "BLEND"}, {"alphaMode": "MASK", "alphaCutoff": 0.25}, {"doubleSided": true})"
    );

    REQUIRE(mesh->materialCount() == 3);

    auto const& materials = mesh->materials();
    REQUIRE(materials.size() == 3);

    REQUIRE(materials[0].alphaMode == AlphaMode::Blend);
    REQUIRE(materials[0].alphaCutoff == Approx(0.5f));
    REQUIRE_FALSE(materials[0].doubleSided);

    REQUIRE(materials[1].alphaMode == AlphaMode::Mask);
    REQUIRE(materials[1].alphaCutoff == Approx(0.25f));
    REQUIRE_FALSE(materials[1].doubleSided);

    REQUIRE(materials[2].alphaMode == AlphaMode::Opaque);
    REQUIRE(materials[2].alphaCutoff == Approx(0.5f));
    REQUIRE(materials[2].doubleSided);
}

TEST_CASE("MeshAsset loadFromBytes rejects empty bytes") {
    std::vector<std::uint8_t> const empty;
    auto const sandbox = repoRoot();
    auto result = MeshAsset::loadFromBytes(
        std::span<std::uint8_t const>(empty.data(), empty.size()), sandbox / "empty.gltf", sandbox
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

TEST_CASE("MeshAsset loadFromBytes rejects Draco compressed primitives") {
    requireGltfError(
        R"({
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
})",
        "Draco compressed primitives are not supported"
    );
}

TEST_CASE("MeshAsset loadFromBytes rejects meshopt compressed accessors") {
    requireGltfError(
        R"({
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
})",
        "meshopt-compressed accessors are not supported"
    );
}

TEST_CASE("MeshAsset loadFromBytes rejects sparse accessors") {
    requireGltfError(
        R"({
    "asset": {"version": "2.0"},
    "buffers": [{
        "byteLength": 42,
        "uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA"
    }],
    "bufferViews": [
        {"buffer": 0, "byteOffset": 0, "byteLength": 36},
        {"buffer": 0, "byteOffset": 36, "byteLength": 6}
    ],
    "accessors": [{
        "bufferView": 0,
        "componentType": 5126,
        "count": 3,
        "type": "VEC3",
        "sparse": {
            "count": 1,
            "indices": {"bufferView": 1, "componentType": 5123, "byteOffset": 0},
            "values": {"bufferView": 0, "byteOffset": 0}
        }
    }, {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}],
    "meshes": [{
        "primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]
    }],
    "nodes": [{"mesh": 0}],
    "scenes": [{"nodes": [0]}],
    "scene": 0
})",
        "sparse accessors are not supported"
    );
}

TEST_CASE("MeshAsset loadFromBytes rejects external buffer outside sandbox") {
    luauapi_test::ScopedTempDir base("luauapi_gltf_sandbox_");
    auto const sandbox = base.path / "sandbox";
    auto const outsideBuffer = base.path / "outside.bin";
    auto const gltfPath = sandbox / "model.gltf";

    REQUIRE(std::filesystem::create_directories(sandbox));

    luauapi_test::writeTestFile(outsideBuffer, kTriangleBufferBytes);

    std::string const gltfJson = externalTriangleGltf("../outside.bin");

    luauapi_test::writeTestFile(gltfPath, gltfJson);

    std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
    auto result = MeshAsset::loadFromBytes(bytes, gltfPath, sandbox);
    REQUIRE(result.isErr());
    REQUIRE(result.unwrapErr().find("escapes sandbox root") != std::string::npos);
}

TEST_CASE("MeshAsset reads valid external buffers and reports missing or oversized files") {
    luauapi_test::ScopedTempDir base("luauapi_gltf_external_buffer_");
    auto const sandbox = base.path / "sandbox";
    auto const gltfPath = sandbox / "model.gltf";
    REQUIRE(std::filesystem::create_directories(sandbox));

    SECTION("valid") {
        luauapi_test::writeTestFile(sandbox / "mesh.bin", kTriangleBufferBytes);
        std::string const gltfJson = externalTriangleGltf("mesh.bin");
        std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
        REQUIRE(MeshAsset::loadFromBytes(bytes, gltfPath, sandbox).isOk());
    }

    SECTION("missing") {
        std::string const gltfJson = externalTriangleGltf("missing.bin");
        std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
        auto result = MeshAsset::loadFromBytes(bytes, gltfPath, sandbox);
        REQUIRE(result.isErr());
        REQUIRE(result.unwrapErr().find("buffer file not found:") != std::string::npos);
    }

    SECTION("oversized") {
        writeSparseFile(sandbox / "large.bin", luax::kMaxFsReadBytes + 1);
        std::string const gltfJson = externalTriangleGltf("large.bin");
        std::vector<std::uint8_t> bytes(gltfJson.begin(), gltfJson.end());
        auto result = MeshAsset::loadFromBytes(bytes, gltfPath, sandbox);
        REQUIRE(result.isErr());
        REQUIRE(result.unwrapErr().find("buffer file exceeds maximum read size") != std::string::npos);
    }
}

TEST_CASE("glTF external image reads enforce the sandbox and size cap") {
    luauapi_test::ScopedTempDir base("luauapi_gltf_external_image_");
    auto const sandbox = base.path / "sandbox";
    auto const assetPath = sandbox / "model.gltf";
    REQUIRE(std::filesystem::create_directories(sandbox));

    cgltf_image image{};
    std::string uri;
    auto read = [&] {
        image.uri = uri.data();
        return readImageEncodedBytes(&image, assetPath, sandbox);
    };

    SECTION("valid") {
        constexpr std::string_view pngBase64 =
            "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU"
            "5ErkJggg==";
        auto decoded =
            geode::utils::base64::decode(pngBase64, geode::utils::base64::Base64Variant::Normal);
        REQUIRE(decoded.isOk());
        luauapi_test::writeTestFile(sandbox / "pixel.png", decoded.unwrap());
        uri = "pixel.png";
        auto result = read();
        REQUIRE(result.isOk());
        REQUIRE(result.unwrap() == decoded.unwrap());
    }

    SECTION("escape") {
        luauapi_test::writeTestFile(base.path / "outside.png", std::array<std::uint8_t, 1>{0});
        uri = "../outside.png";
        auto result = read();
        REQUIRE(result.isErr());
        REQUIRE(result.unwrapErr() == "image path escapes sandbox root");
    }

    SECTION("missing") {
        uri = "missing.png";
        auto result = read();
        REQUIRE(result.isErr());
        REQUIRE(result.unwrapErr().find("image file not found:") == 0);
    }

    SECTION("oversized") {
        writeSparseFile(sandbox / "large.png", luax::kMaxFsReadBytes + 1);
        uri = "large.png";
        auto result = read();
        REQUIRE(result.isErr());
        REQUIRE(result.unwrapErr() == "image file exceeds maximum read size");
    }
}
