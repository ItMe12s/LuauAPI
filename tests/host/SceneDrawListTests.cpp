#include "render3d/assets/MeshAsset.hpp"
#include "render3d/gpu/SceneDrawList.hpp"
#include "render3d/types/Frustum.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    using namespace luax::render3d;
    using Catch::Approx;

    std::shared_ptr<MeshAsset> requireMesh(LoadResult<std::shared_ptr<MeshAsset>> result) {
        REQUIRE(result.isOk());
        return result.unwrap();
    }

    GpuMesh makeGpuMesh(unsigned int vbo, unsigned int ibo, unsigned int indexCount, int materialIndex = -1) {
        GpuMesh mesh{};
        mesh.primitives.push_back(
            GpuPrimitive{.vbo = vbo, .ibo = ibo, .indexCount = indexCount, .materialIndex = materialIndex}
        );
        return mesh;
    }

    GpuMeshResolver fixedGpuMesh(GpuMesh* mesh) {
        return [mesh](std::uint64_t, MeshAsset const&) -> GpuMesh* { return mesh; };
    }
} // namespace

TEST_CASE("shaderAlphaCutoff returns cutoff only for mask mode") {
    REQUIRE(shaderAlphaCutoff(0, 0.5f) == Approx(0.0f));
    REQUIRE(shaderAlphaCutoff(1, 0.35f) == Approx(0.35f));
    REQUIRE(shaderAlphaCutoff(2, 0.35f) == Approx(0.0f));
}

TEST_CASE("sameInstancedBatch matches primitive texture material and sidedness") {
    GpuPrimitive primA{.vbo = 1, .ibo = 2, .indexCount = 3};
    GpuPrimitive primB{.vbo = 9, .ibo = 2, .indexCount = 3};

    SceneDrawItem base{};
    base.prim = &primA;
    base.boundTexture = 10;
    base.baseColor = glm::vec4(1.0f, 0.5f, 0.25f, 1.0f);
    base.alphaMode = 1;
    base.alphaCutoff = 0.4f;
    base.doubleSided = false;

    SceneDrawItem match = base;
    REQUIRE(sameInstancedBatch(base, match));

    SceneDrawItem differentPrim = base;
    differentPrim.prim = &primB;
    REQUIRE_FALSE(sameInstancedBatch(base, differentPrim));

    SceneDrawItem differentTexture = base;
    differentTexture.boundTexture = 11;
    REQUIRE_FALSE(sameInstancedBatch(base, differentTexture));

    SceneDrawItem differentCutoff = base;
    differentCutoff.alphaCutoff = 0.2f;
    REQUIRE_FALSE(sameInstancedBatch(base, differentCutoff));

    SceneDrawItem differentSided = base;
    differentSided.doubleSided = true;
    REQUIRE_FALSE(sameInstancedBatch(base, differentSided));
}

TEST_CASE("sortOpaqueDrawItems orders by bound texture then vbo") {
    GpuPrimitive primLowVbo{.vbo = 5, .ibo = 1, .indexCount = 3};
    GpuPrimitive primHighVbo{.vbo = 20, .ibo = 1, .indexCount = 3};

    std::vector<SceneDrawItem> items(3);
    items[0].prim = &primHighVbo;
    items[0].boundTexture = 2;
    items[1].prim = &primLowVbo;
    items[1].boundTexture = 1;
    items[2].prim = &primHighVbo;
    items[2].boundTexture = 1;

    sortOpaqueDrawItems(items);

    REQUIRE(items[0].boundTexture == 1);
    REQUIRE(items[0].prim->vbo == 5);
    REQUIRE(items[1].boundTexture == 1);
    REQUIRE(items[1].prim->vbo == 20);
    REQUIRE(items[2].boundTexture == 2);
}

TEST_CASE("sortBlendDrawItems orders by ascending view depth") {
    std::vector<SceneDrawItem> items{
        SceneDrawItem{.viewDepth = 2.0f},
        SceneDrawItem{.viewDepth = -1.0f},
        SceneDrawItem{.viewDepth = 0.5f},
    };

    sortBlendDrawItems(items);

    REQUIRE(items[0].viewDepth == Approx(-1.0f));
    REQUIRE(items[1].viewDepth == Approx(0.5f));
    REQUIRE(items[2].viewDepth == Approx(2.0f));
}

TEST_CASE("resolveSceneDrawTexture returns mesh image texture when not self attachment") {
    GpuMesh mesh{};
    mesh.textures = {77u};

    SceneDrawItem item{};
    item.texSource = &mesh;
    item.imageIndex = 0;

    TextureResolver const resolveTexture = [](std::uint64_t, TextureAsset const&) -> unsigned int {
        return 0u;
    };
    REQUIRE(resolveSceneDrawTexture(item, resolveTexture, 42) == 77u);
}

TEST_CASE("resolveSceneDrawTexture suppresses self FBO feedback texture") {
    GpuMesh mesh{};
    mesh.textures = {42u};

    SceneDrawItem item{};
    item.texSource = &mesh;
    item.imageIndex = 0;

    TextureResolver const resolveTexture = [](std::uint64_t, TextureAsset const&) -> unsigned int {
        return 0u;
    };
    REQUIRE(resolveSceneDrawTexture(item, resolveTexture, 42) == 0u);
}

TEST_CASE("buildSceneDrawLists skips instances outside frustum") {
    auto mesh = requireMesh(MeshAsset::fromBuffers(
        {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
        },
        {},
        {},
        {0, 1, 2}
    ));

    GpuMesh gpuMesh = makeGpuMesh(1, 2, 3);
    glm::mat4 const view = glm::mat4(1.0f);
    Frustum const frustum = Frustum::fromViewProj(glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f));

    ViewportInstance inside{};
    inside.meshId = 1;
    inside.mesh = mesh;
    inside.transform.position = glm::vec3(0.0f, 0.0f, -5.0f);

    ViewportInstance outside{};
    outside.meshId = 2;
    outside.mesh = mesh;
    outside.transform.position = glm::vec3(50.0f, 0.0f, -5.0f);

    std::map<int, ViewportInstance> instances{
        {1, inside},
        {2, outside},
    };

    SceneDrawLists const lists =
        buildSceneDrawLists(instances, view, frustum, fixedGpuMesh(&gpuMesh));

    REQUIRE(lists.opaque.size() == 1);
    REQUIRE(lists.blend.empty());
    REQUIRE(lists.opaque.front().model == inside.transform.toMat4());
}

TEST_CASE("buildSceneDrawLists routes alpha blend materials to blend list") {
    auto mesh = requireMesh(MeshAsset::fromBuffers(
        {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
        },
        {},
        {},
        {0, 1, 2}
    ));

    GpuMesh gpuMesh = makeGpuMesh(1, 2, 3);
    glm::mat4 const view = glm::mat4(1.0f);
    Frustum const frustum = Frustum::fromViewProj(glm::mat4(1.0f));

    auto blendMaterial = std::make_shared<Material>();
    blendMaterial->alphaMode = 2;
    blendMaterial->baseColorFactor = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);

    ViewportInstance instance{};
    instance.meshId = 1;
    instance.mesh = mesh;
    instance.materialOverride = blendMaterial;

    std::map<int, ViewportInstance> instances{{1, instance}};

    SceneDrawLists const lists =
        buildSceneDrawLists(instances, view, frustum, fixedGpuMesh(&gpuMesh));

    REQUIRE(lists.opaque.empty());
    REQUIRE(lists.blend.size() == 1);
    REQUIRE(lists.blend.front().alphaMode == 2);
    REQUIRE(lists.blend.front().baseColor.a == Approx(0.5f));
}

TEST_CASE("buildSceneDrawLists applies primitive override over material override") {
    auto mesh = requireMesh(MeshAsset::fromBuffers(
        {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
        },
        {},
        {},
        {0, 1, 2}
    ));

    GpuMesh gpuMesh = makeGpuMesh(1, 2, 3);
    glm::mat4 const view = glm::mat4(1.0f);
    Frustum const frustum = Frustum::fromViewProj(glm::mat4(1.0f));

    auto instanceMaterial = std::make_shared<Material>();
    instanceMaterial->baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    auto primitiveMaterial = std::make_shared<Material>();
    primitiveMaterial->baseColorFactor = glm::vec4(0.1f, 0.2f, 0.3f, 1.0f);
    primitiveMaterial->doubleSided = true;

    ViewportInstance instance{};
    instance.meshId = 1;
    instance.mesh = mesh;
    instance.materialOverride = instanceMaterial;
    instance.primitiveOverrides[0] = primitiveMaterial;

    std::map<int, ViewportInstance> instances{{1, instance}};

    SceneDrawLists const lists =
        buildSceneDrawLists(instances, view, frustum, fixedGpuMesh(&gpuMesh));

    REQUIRE(lists.opaque.size() == 1);
    REQUIRE(lists.opaque.front().baseColor.r == Approx(0.1f));
    REQUIRE(lists.opaque.front().doubleSided);
}

TEST_CASE("buildSceneDrawLists skips invalid gpu primitives") {
    auto mesh = requireMesh(MeshAsset::fromBuffers(
        {
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
        },
        {},
        {},
        {0, 1, 2}
    ));

    GpuMesh gpuMesh = makeGpuMesh(0, 0, 0);
    glm::mat4 const view = glm::mat4(1.0f);
    Frustum const frustum = Frustum::fromViewProj(glm::mat4(1.0f));

    ViewportInstance instance{};
    instance.meshId = 1;
    instance.mesh = mesh;

    std::map<int, ViewportInstance> instances{{1, instance}};

    SceneDrawLists const lists =
        buildSceneDrawLists(instances, view, frustum, fixedGpuMesh(&gpuMesh));

    REQUIRE(lists.opaque.empty());
    REQUIRE(lists.blend.empty());
}
