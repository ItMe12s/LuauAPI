#pragma once

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
struct cgltf_image;
struct cgltf_options;

namespace luax::render3d {

    template <class T>
    class LoadResult {
    public:
        static LoadResult ok(T value) {
            LoadResult result;
            result.m_value = std::move(value);
            return result;
        }

        static LoadResult err(std::string message) {
            LoadResult result;
            result.m_error = std::move(message);
            return result;
        }

        bool isOk() const {
            return m_value.has_value();
        }

        bool isErr() const {
            return !isOk();
        }

        T& unwrap() {
            return *m_value;
        }

        T const& unwrap() const {
            return *m_value;
        }

        std::string const& unwrapErr() const {
            return m_error;
        }

    private:
        std::optional<T> m_value;
        std::string m_error;
    };

    struct ImageData {
        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> rgba;
    };

    struct MaterialData {
        glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        int imageIndex = -1;
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

    class MeshAsset {
    public:
        static LoadResult<std::shared_ptr<MeshAsset>> loadFromFile(std::filesystem::path const& path);

        static LoadResult<std::shared_ptr<MeshAsset>> loadFromBytes(
            std::span<std::uint8_t const> bytes, std::filesystem::path const& assetPath,
            std::filesystem::path const& sandboxRoot
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
            ::cgltf_data const* data, MeshAsset& asset, ::cgltf_options const& options,
            std::filesystem::path const& assetPath, std::filesystem::path const& sandboxRoot
        );
        static LoadResult<int> resolveImageIndex(
            ::cgltf_image const* image, std::filesystem::path const& assetPath,
            std::filesystem::path const& sandboxRoot, MeshAsset& asset,
            std::unordered_map<::cgltf_image const*, int>& imageIndices
        );
        static std::optional<std::string> extractSceneMeshes(::cgltf_data const* data, MeshAsset& asset);

        std::vector<MeshPrimitive> m_primitives;
        std::vector<MaterialData> m_materials;
        std::vector<ImageData> m_images;
        BoundingBox m_bounds;
        std::size_t m_vertexCount = 0;
    };

    class MeshRegistry {
    public:
        static MeshRegistry& instance();

        std::uint64_t registerMesh(std::shared_ptr<MeshAsset> mesh);
        void release(std::uint64_t id);
        std::shared_ptr<MeshAsset> get(std::uint64_t id) const;

    private:
        MeshRegistry() = default;

        std::uint64_t m_nextId = 1;
        std::unordered_map<std::uint64_t, std::shared_ptr<MeshAsset>> m_meshes;
    };

} // namespace luax::render3d
