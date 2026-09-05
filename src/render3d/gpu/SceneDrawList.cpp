#include "render3d/gpu/SceneDrawList.hpp"

#include "render3d/assets/MeshAsset.hpp"
#include "render3d/types/Frustum.hpp"

#include <algorithm>
#include <ranges>

namespace luax::render3d {
    namespace {
        void applyRuntimeMaterial(
            SceneDrawItem& item, Material const& overrideMat, GpuMeshResolver& resolveGpuMesh
        ) {
            item.baseColor = overrideMat.baseColorFactor;
            item.alphaMode = overrideMat.alphaMode;
            item.alphaCutoff = overrideMat.alphaCutoff;
            item.doubleSided = overrideMat.doubleSided;
            item.textureAsset = nullptr;
            item.imageIndex = overrideMat.imageIndex;
            if (overrideMat.texture != nullptr) {
                item.textureAsset = overrideMat.texture.get();
                item.imageIndex = -1;
            }
            else if (item.imageIndex >= 0) {
                if (overrideMat.sourceMesh != nullptr) {
                    item.texSource = resolveGpuMesh(*overrideMat.sourceMesh);
                    if (item.texSource == nullptr) {
                        item.imageIndex = -1;
                    }
                }
                else {
                    item.imageIndex = -1;
                }
            }
        }
    } // namespace

    float shaderAlphaCutoff(AlphaMode alphaMode, float alphaCutoff) {
        return alphaMode == AlphaMode::Mask ? alphaCutoff : 0.0f;
    }

    void sortOpaqueDrawItems(std::vector<SceneDrawItem>& items) {
        std::ranges::sort(items, [](SceneDrawItem const& a, SceneDrawItem const& b) {
            if (a.boundTexture != b.boundTexture) {
                return a.boundTexture < b.boundTexture;
            }
            return a.prim->vbo < b.prim->vbo;
        });
    }

    void sortBlendDrawItems(std::vector<SceneDrawItem>& items) {
        std::ranges::sort(items, [](SceneDrawItem const& a, SceneDrawItem const& b) {
            return a.viewDepth < b.viewDepth;
        });
    }

    unsigned int resolveSceneDrawTexture(
        SceneDrawItem const& item, TextureResolver& resolveTexture, int selfColorTexture
    ) {
        unsigned int resolved = 0;
        if (item.textureAsset != nullptr) {
            resolved = resolveTexture(*item.textureAsset);
        }
        else if (
            item.imageIndex >= 0 && item.texSource != nullptr &&
            static_cast<std::size_t>(item.imageIndex) < item.texSource->textures.size()
        ) {
            resolved = item.texSource->textures[static_cast<std::size_t>(item.imageIndex)];
        }
        if (resolved == static_cast<unsigned int>(selfColorTexture)) {
            return 0;
        }
        return resolved;
    }

    SceneDrawLists buildSceneDrawLists(
        std::unordered_map<int, ViewportInstance> const& instances, glm::mat4 const& view,
        Frustum const& frustum, GpuMeshResolver& resolveGpuMesh
    ) {
        SceneDrawLists lists{};
        lists.opaque.reserve(instances.size() * 4);
        lists.blend.reserve(instances.size());

        for (auto const& [instanceId, instance] : instances) {
            (void)instanceId;
            if (instance.mesh == nullptr) {
                continue;
            }
            GpuMesh* gpuMesh = resolveGpuMesh(*instance.mesh);
            if (gpuMesh == nullptr) {
                continue;
            }

            glm::mat4 const model = instance.transform.toMat4();
            BoundingBox const& bounds = instance.mesh->boundingBox();
            if (!bounds.empty) {
                glm::vec3 const center = (bounds.min + bounds.max) * 0.5f;
                float const radius = glm::length(bounds.max - bounds.min) * 0.5f;
                glm::vec3 const worldCenter = glm::vec3(model * glm::vec4(center, 1.0f));
                if (!frustum.intersectsSphere(worldCenter, radius)) {
                    continue;
                }
            }

            glm::vec4 const viewPos = view * model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            float const viewDepth = viewPos.z;
            auto const& materials = instance.mesh->materials();

            for (std::size_t primitiveIndex = 0; primitiveIndex < gpuMesh->primitives.size();
                 ++primitiveIndex) {
                auto const& primitive = gpuMesh->primitives[primitiveIndex];
                if (primitive.vbo == 0 || primitive.ibo == 0 || primitive.indexCount == 0) {
                    continue;
                }

                SceneDrawItem item{};
                item.prim = &primitive;
                item.tint = instance.color;
                item.model = model;
                item.viewDepth = viewDepth;
                item.texSource = gpuMesh;

                auto const primOverrideIt =
                    instance.primitiveOverrides.find(static_cast<int>(primitiveIndex));
                if (primOverrideIt != instance.primitiveOverrides.end() &&
                    primOverrideIt->second != nullptr) {
                    applyRuntimeMaterial(item, *primOverrideIt->second, resolveGpuMesh);
                }
                else if (instance.materialOverride != nullptr) {
                    applyRuntimeMaterial(item, *instance.materialOverride, resolveGpuMesh);
                }
                else if (
                    primitive.materialIndex >= 0 &&
                    static_cast<std::size_t>(primitive.materialIndex) < materials.size()
                ) {
                    auto const& material =
                        materials[static_cast<std::size_t>(primitive.materialIndex)];
                    item.baseColor = material.baseColorFactor;
                    item.imageIndex = material.imageIndex;
                    item.alphaMode = material.alphaMode;
                    item.alphaCutoff = material.alphaCutoff;
                    item.doubleSided = material.doubleSided;
                }

                if (item.alphaMode == AlphaMode::Blend) {
                    lists.blend.push_back(item);
                }
                else {
                    lists.opaque.push_back(item);
                }
            }
        }

        return lists;
    }

} // namespace luax::render3d
