#pragma once

#include "render3d/assets/AssetRegistry.hpp"

#include <Geode/Result.hpp>
#include <cstdint>
#include <filesystem>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

struct cgltf_data;

namespace luax::render3d {

    struct ImageData {
        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> rgba;
    };

    struct MaterialData {
        glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        int imageIndex = -1;
        int alphaMode = 0;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
    };

    struct MeshPrimitive {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
        std::vector<std::uint32_t> indices;
        int materialIndex = -1;
    };

    struct BoundingBox {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
        bool empty = true;
    };

    constexpr std::size_t kMaxProceduralMeshVertices = 200'000;

    class MeshAsset {
    public:
        static geode::Result<std::shared_ptr<MeshAsset>> loadFromFile(std::filesystem::path const& path);

        static geode::Result<std::shared_ptr<MeshAsset>> loadFromBytes(
            std::span<std::uint8_t const> bytes, std::filesystem::path const& assetPath,
            std::filesystem::path const& sandboxRoot
        );

        static geode::Result<std::shared_ptr<MeshAsset>> fromBuffers(
            std::vector<glm::vec3> positions, std::vector<glm::vec3> normals,
            std::vector<glm::vec2> uvs, std::vector<std::uint32_t> indices
        );

        std::size_t vertexCount() const;
        std::size_t primitiveCount() const;
        BoundingBox const& boundingBox() const;
        std::vector<MeshPrimitive> const& primitives() const;
        std::vector<MaterialData> const& materials() const;
        std::size_t materialCount() const;
        std::vector<ImageData> const& images() const;

    private:
        MeshAsset() = default;

        void addPrimitive(MeshPrimitive primitive);
        static std::optional<std::string> extractMaterials(
            ::cgltf_data const* data, MeshAsset& asset, std::filesystem::path const& assetPath,
            std::filesystem::path const& sandboxRoot
        );
        static std::optional<std::string> extractSceneMeshes(::cgltf_data const* data, MeshAsset& asset);

        std::vector<MeshPrimitive> m_primitives;
        std::vector<MaterialData> m_materials;
        std::vector<ImageData> m_images;
        BoundingBox m_bounds;
        std::size_t m_vertexCount = 0;
    };

    using MeshRegistry = AssetRegistry<MeshAsset>;

} // namespace luax::render3d
