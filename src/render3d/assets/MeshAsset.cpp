#include "render3d/assets/MeshAsset.hpp"

#include "core/Config.hpp"
#include "render3d/assets/GltfIo.hpp"
#include "render3d/assets/ImageDecode.hpp"
#include "require/PathSandbox.hpp"

#include <Geode/utils/file.hpp>
#include <cgltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    using namespace luax;
    using namespace luax::render3d;

    char const* cgltfResultMessage(cgltf_result result) {
        switch (result) {
            case cgltf_result_success: return "success";
            case cgltf_result_data_too_short: return "data too short";
            case cgltf_result_unknown_format: return "unknown format";
            case cgltf_result_invalid_json: return "invalid json";
            case cgltf_result_invalid_gltf: return "invalid gltf";
            case cgltf_result_invalid_options: return "invalid options";
            case cgltf_result_file_not_found: return "file not found";
            case cgltf_result_io_error: return "io error";
            case cgltf_result_out_of_memory: return "out of memory";
            case cgltf_result_legacy_gltf: return "legacy gltf";
            default: return "unknown cgltf error";
        }
    }

    bool accessorUsesMeshopt(cgltf_accessor const* accessor) {
        return accessor != nullptr && accessor->buffer_view != nullptr &&
            accessor->buffer_view->has_meshopt_compression;
    }

    template <class VecT>
    LoadResult<std::vector<VecT>> unpackVecAttribute(cgltf_accessor const* accessor, char const* label) {
        constexpr std::size_t kComponents = static_cast<std::size_t>(VecT::length());
        constexpr cgltf_type kExpectedType = kComponents == 2 ? cgltf_type_vec2 : cgltf_type_vec3;
        char const* const kTypeName = kComponents == 2 ? "vec2" : "vec3";

        if (accessor == nullptr) {
            return LoadResult<std::vector<VecT>>::err(std::string(label) + " accessor is missing");
        }

        if (accessor->is_sparse) {
            return LoadResult<std::vector<VecT>>::err(
                std::string(label) + " sparse accessors are not supported"
            );
        }

        if (accessorUsesMeshopt(accessor)) {
            return LoadResult<std::vector<VecT>>::err(
                std::string(label) + " meshopt-compressed accessors are not supported"
            );
        }

        if (accessor->type != kExpectedType) {
            return LoadResult<std::vector<VecT>>::err(
                std::string(label) + " accessor must be " + kTypeName
            );
        }

        cgltf_size const floatCount = cgltf_accessor_unpack_floats(accessor, nullptr, 0);
        if (floatCount == 0 || floatCount % kComponents != 0) {
            return LoadResult<std::vector<VecT>>::err(
                std::string(label) + " accessor has no vertices"
            );
        }

        std::vector<float> floats(floatCount);
        cgltf_accessor_unpack_floats(accessor, floats.data(), floatCount);

        std::vector<VecT> values(floatCount / kComponents);
        for (std::size_t i = 0; i < values.size(); ++i) {
            for (std::size_t c = 0; c < kComponents; ++c) {
                values[i][static_cast<int>(c)] = floats[i * kComponents + c];
            }
        }

        return LoadResult<std::vector<VecT>>::ok(std::move(values));
    }

    LoadResult<std::vector<std::uint32_t>> unpackIndices(cgltf_primitive const& primitive) {
        if (primitive.indices == nullptr) {
            auto const* position = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
            if (position == nullptr) {
                return LoadResult<std::vector<std::uint32_t>>::err(
                    "primitive is missing position data"
                );
            }

            std::vector<std::uint32_t> indices(position->count);
            for (cgltf_size i = 0; i < position->count; ++i) {
                indices[static_cast<std::size_t>(i)] = static_cast<std::uint32_t>(i);
            }
            return LoadResult<std::vector<std::uint32_t>>::ok(std::move(indices));
        }

        if (primitive.indices->is_sparse) {
            return LoadResult<std::vector<std::uint32_t>>::err(
                "sparse index accessors are not supported"
            );
        }

        if (accessorUsesMeshopt(primitive.indices)) {
            return LoadResult<std::vector<std::uint32_t>>::err(
                "meshopt-compressed index accessors are not supported"
            );
        }

        cgltf_size const indexCount =
            cgltf_accessor_unpack_indices(primitive.indices, nullptr, sizeof(std::uint32_t), 0);
        if (indexCount == 0) {
            return LoadResult<std::vector<std::uint32_t>>::err("primitive index accessor is empty");
        }

        std::vector<std::uint32_t> indices(indexCount);
        cgltf_size const unpacked = cgltf_accessor_unpack_indices(
            primitive.indices, indices.data(), sizeof(std::uint32_t), indexCount
        );
        if (unpacked != indexCount) {
            return LoadResult<std::vector<std::uint32_t>>::err("failed to unpack primitive indices");
        }

        return LoadResult<std::vector<std::uint32_t>>::ok(std::move(indices));
    }

    LoadResult<MeshPrimitive> extractPrimitive(
        cgltf_data const* data, cgltf_primitive const& primitive, glm::mat4 const& worldMatrix
    ) {
        if (primitive.has_draco_mesh_compression) {
            return LoadResult<MeshPrimitive>::err("Draco compressed primitives are not supported");
        }

        if (primitive.type != cgltf_primitive_type_triangles) {
            return LoadResult<MeshPrimitive>::err("only triangle primitives are supported");
        }

        auto const* positionAccessor =
            cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
        auto positionsResult = unpackVecAttribute<glm::vec3>(positionAccessor, "position");
        if (positionsResult.isErr()) {
            return LoadResult<MeshPrimitive>::err(positionsResult.unwrapErr());
        }

        auto positions = std::move(positionsResult.unwrap());
        glm::mat3 const normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));

        for (auto& position : positions) {
            glm::vec4 const transformed = worldMatrix * glm::vec4(position, 1.0f);
            position = glm::vec3(transformed);
        }

        std::vector<glm::vec3> normals;
        auto const* normalAccessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0);
        if (normalAccessor != nullptr) {
            auto normalsResult = unpackVecAttribute<glm::vec3>(normalAccessor, "normal");
            if (normalsResult.isErr()) {
                return LoadResult<MeshPrimitive>::err(normalsResult.unwrapErr());
            }

            normals = std::move(normalsResult.unwrap());
            if (normals.size() != positions.size()) {
                return LoadResult<MeshPrimitive>::err(
                    "position and normal vertex counts do not match"
                );
            }

            for (auto& normal : normals) {
                normal = glm::normalize(normalMatrix * normal);
            }
        }
        else {
            normals.assign(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        auto indicesResult = unpackIndices(primitive);
        if (indicesResult.isErr()) {
            return LoadResult<MeshPrimitive>::err(indicesResult.unwrapErr());
        }

        std::vector<glm::vec2> texcoords;
        auto const* texcoordAccessor =
            cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0);
        if (texcoordAccessor != nullptr) {
            auto texcoordsResult = unpackVecAttribute<glm::vec2>(texcoordAccessor, "texcoord");
            if (texcoordsResult.isErr()) {
                return LoadResult<MeshPrimitive>::err(texcoordsResult.unwrapErr());
            }

            texcoords = std::move(texcoordsResult.unwrap());
            if (texcoords.size() != positions.size()) {
                return LoadResult<MeshPrimitive>::err(
                    "position and texcoord vertex counts do not match"
                );
            }
        }

        int materialIndex = -1;
        if (primitive.material != nullptr) {
            if (data->materials == nullptr || data->materials_count == 0) {
                return LoadResult<MeshPrimitive>::err("primitive references a missing material");
            }

            materialIndex = static_cast<int>(primitive.material - data->materials);
            if (materialIndex < 0 || static_cast<cgltf_size>(materialIndex) >= data->materials_count) {
                return LoadResult<MeshPrimitive>::err("primitive references an invalid material");
            }
        }

        MeshPrimitive meshPrimitive;
        meshPrimitive.positions = std::move(positions);
        meshPrimitive.normals = std::move(normals);
        meshPrimitive.texcoords = std::move(texcoords);
        meshPrimitive.indices = std::move(indicesResult.unwrap());
        meshPrimitive.materialIndex = materialIndex;
        return LoadResult<MeshPrimitive>::ok(std::move(meshPrimitive));
    }

    LoadResult<int> resolveImageIndex(
        cgltf_image const* image, std::filesystem::path const& assetPath,
        std::filesystem::path const& sandboxRoot, std::vector<ImageData>& images,
        std::unordered_map<cgltf_image const*, int>& imageIndices
    ) {
        auto const existing = imageIndices.find(image);
        if (existing != imageIndices.end()) {
            return LoadResult<int>::ok(existing->second);
        }

        auto encodedResult = readImageEncodedBytes(image, assetPath, sandboxRoot);
        if (encodedResult.isErr()) {
            return LoadResult<int>::err(encodedResult.unwrapErr());
        }

        auto decodeResult = decodeImageRgba8(encodedResult.unwrap());
        if (decodeResult.isErr()) {
            return LoadResult<int>::err(decodeResult.unwrapErr());
        }

        int const index = static_cast<int>(images.size());
        images.push_back(std::move(decodeResult.unwrap()));
        imageIndices.emplace(image, index);
        return LoadResult<int>::ok(index);
    }

    void computeFlatNormals(
        std::vector<glm::vec3> const& positions, std::vector<std::uint32_t> const& indices,
        std::vector<glm::vec3>& outNormals
    ) {
        outNormals.assign(positions.size(), glm::vec3(0.0f));

        for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
            std::uint32_t const i0 = indices[i];
            std::uint32_t const i1 = indices[i + 1];
            std::uint32_t const i2 = indices[i + 2];
            glm::vec3 const edge1 = positions[i1] - positions[i0];
            glm::vec3 const edge2 = positions[i2] - positions[i0];
            glm::vec3 faceNormal = glm::cross(edge1, edge2);
            float const len = glm::length(faceNormal);
            if (len > 0.0f) {
                faceNormal /= len;
            }

            outNormals[i0] += faceNormal;
            outNormals[i1] += faceNormal;
            outNormals[i2] += faceNormal;
        }

        for (auto& normal : outNormals) {
            float const len = glm::length(normal);
            if (len > 0.0f) {
                normal /= len;
            }
            else {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }
    }
} // namespace

namespace luax::render3d {

    std::optional<std::string> MeshAsset::extractMaterials(
        ::cgltf_data const* data, MeshAsset& asset, std::filesystem::path const& assetPath,
        std::filesystem::path const& sandboxRoot
    ) {
        std::unordered_map<cgltf_image const*, int> imageIndices;
        asset.m_materials.clear();
        asset.m_images.clear();
        asset.m_materials.reserve(static_cast<std::size_t>(data->materials_count));

        for (cgltf_size materialIndex = 0; materialIndex < data->materials_count; ++materialIndex) {
            cgltf_material const& material = data->materials[materialIndex];
            MaterialData materialData;

            if (material.has_pbr_metallic_roughness) {
                cgltf_pbr_metallic_roughness const& pbr = material.pbr_metallic_roughness;
                materialData.baseColorFactor = glm::vec4(
                    pbr.base_color_factor[0],
                    pbr.base_color_factor[1],
                    pbr.base_color_factor[2],
                    pbr.base_color_factor[3]
                );

                cgltf_texture_view const& baseColorTexture = pbr.base_color_texture;
                if (baseColorTexture.texture != nullptr) {
                    cgltf_texture const* texture = baseColorTexture.texture;
                    if (texture->has_basisu || texture->has_webp) {
                        return "KHR texture extensions are not supported";
                    }

                    if (texture->image == nullptr) {
                        return "base color texture has no image";
                    }

                    auto imageIndexResult = resolveImageIndex(
                        texture->image, assetPath, sandboxRoot, asset.m_images, imageIndices
                    );
                    if (imageIndexResult.isErr()) {
                        return imageIndexResult.unwrapErr();
                    }

                    materialData.imageIndex = imageIndexResult.unwrap();
                }
            }

            switch (material.alpha_mode) {
                case cgltf_alpha_mode_opaque: materialData.alphaMode = 0; break;
                case cgltf_alpha_mode_mask: materialData.alphaMode = 1; break;
                case cgltf_alpha_mode_blend: materialData.alphaMode = 2; break;
                default: materialData.alphaMode = 0; break;
            }
            materialData.alphaCutoff = material.alpha_cutoff;
            materialData.doubleSided = material.double_sided != 0;

            asset.m_materials.push_back(materialData);
        }

        return std::nullopt;
    }

    std::optional<std::string> MeshAsset::extractSceneMeshes(::cgltf_data const* data, MeshAsset& asset) {
        bool foundMesh = false;

        for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex) {
            cgltf_node const* node = &data->nodes[nodeIndex];
            if (node->mesh == nullptr) {
                continue;
            }

            float worldMatrixRaw[16] = {};
            cgltf_node_transform_world(node, worldMatrixRaw);
            glm::mat4 const worldMatrix = glm::make_mat4(worldMatrixRaw);

            for (cgltf_size primitiveIndex = 0; primitiveIndex < node->mesh->primitives_count;
                 ++primitiveIndex) {
                auto const& primitive = node->mesh->primitives[primitiveIndex];
                auto primitiveResult = extractPrimitive(data, primitive, worldMatrix);
                if (primitiveResult.isErr()) {
                    return primitiveResult.unwrapErr();
                }

                auto meshPrimitive = std::move(primitiveResult.unwrap());
                if (meshPrimitive.materialIndex >= 0 &&
                    static_cast<std::size_t>(meshPrimitive.materialIndex) < asset.m_materials.size()) {
                    auto const& material =
                        asset.m_materials[static_cast<std::size_t>(meshPrimitive.materialIndex)];
                    if (material.imageIndex >= 0 && meshPrimitive.texcoords.empty()) {
                        return "textures require TEXCOORD_0";
                    }
                }

                asset.addPrimitive(std::move(meshPrimitive));
                foundMesh = true;
            }
        }

        if (!foundMesh) {
            return "glTF file contains no mesh primitives";
        }

        return std::nullopt;
    }

    LoadResult<std::shared_ptr<MeshAsset>> MeshAsset::loadFromFile(std::filesystem::path const& path) {
        std::error_code ec;
        auto const fileSize = std::filesystem::file_size(path, ec);
        if (ec) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(
                "glTF file cannot be read: " + filesystemPathString(path)
            );
        }

        if (fileSize > kMaxFsReadBytes) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err("glTF file exceeds maximum read size");
        }

        auto bytesResult = geode::utils::file::readBinary(path);
        if (bytesResult.isErr()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(
                "glTF file cannot be read: " + filesystemPathString(path)
            );
        }

        auto bytes = std::move(bytesResult.unwrap());
        return loadFromBytes(bytes, path, path.parent_path());
    }

    LoadResult<std::shared_ptr<MeshAsset>> MeshAsset::loadFromBytes(
        std::span<std::uint8_t const> bytes, std::filesystem::path const& assetPath,
        std::filesystem::path const& sandboxRoot
    ) {
        if (bytes.empty()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err("glTF data is empty");
        }

        if (bytes.size() > kMaxFsReadBytes) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err("glTF data exceeds maximum read size");
        }

        auto rootResult = canonicalSandboxRoot(sandboxRoot);
        if (rootResult.isErr()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(rootResult.unwrapErr());
        }

        SandboxFileContext fileContext{rootResult.unwrap(), {}};

        cgltf_options options{};
        configureSandboxFileIo(options, fileContext);

        ::cgltf_data* data = nullptr;
        cgltf_result parseResult =
            cgltf_parse(&options, bytes.data(), static_cast<cgltf_size>(bytes.size()), &data);
        if (parseResult != cgltf_result_success) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(
                std::string("failed to parse glTF: ") + cgltfResultMessage(parseResult)
            );
        }

        std::string const assetPathText = filesystemPathString(assetPath);
        cgltf_result bufferResult = cgltf_load_buffers(&options, data, assetPathText.c_str());
        if (bufferResult != cgltf_result_success) {
            std::string message = "failed to load glTF buffers: ";
            message += fileContext.lastError.empty() ? cgltfResultMessage(bufferResult) :
                                                       fileContext.lastError;
            cgltf_free(data);
            return LoadResult<std::shared_ptr<MeshAsset>>::err(std::move(message));
        }

        auto mesh = std::shared_ptr<MeshAsset>(new MeshAsset());
        auto materialError = MeshAsset::extractMaterials(data, *mesh, assetPath, rootResult.unwrap());
        if (materialError.has_value()) {
            cgltf_free(data);
            return LoadResult<std::shared_ptr<MeshAsset>>::err(*materialError);
        }

        auto extractError = MeshAsset::extractSceneMeshes(data, *mesh);
        cgltf_free(data);

        if (extractError.has_value()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(*extractError);
        }

        return LoadResult<std::shared_ptr<MeshAsset>>::ok(std::move(mesh));
    }

    LoadResult<std::shared_ptr<MeshAsset>> MeshAsset::fromBuffers(
        std::vector<glm::vec3> positions, std::vector<glm::vec3> normals,
        std::vector<glm::vec2> uvs, std::vector<std::uint32_t> indices
    ) {
        if (positions.empty()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err("positions are empty");
        }

        if (positions.size() > kMaxProceduralMeshVertices) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(
                "positions exceed maximum vertex count"
            );
        }

        if (indices.empty() || indices.size() % 3 != 0) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(
                "indices must contain a multiple of three triangle indices"
            );
        }

        if (!normals.empty() && normals.size() != positions.size()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(
                "normals length must match positions or be omitted"
            );
        }

        if (!uvs.empty() && uvs.size() != positions.size()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(
                "uvs length must match positions or be omitted"
            );
        }

        for (auto const index : indices) {
            if (index >= positions.size()) {
                return LoadResult<std::shared_ptr<MeshAsset>>::err("index out of range");
            }
        }

        if (normals.empty()) {
            computeFlatNormals(positions, indices, normals);
        }

        MeshPrimitive primitive;
        primitive.positions = std::move(positions);
        primitive.normals = std::move(normals);
        primitive.texcoords = std::move(uvs);
        primitive.indices = std::move(indices);
        primitive.materialIndex = -1;

        auto mesh = std::shared_ptr<MeshAsset>(new MeshAsset());
        mesh->addPrimitive(std::move(primitive));
        return LoadResult<std::shared_ptr<MeshAsset>>::ok(std::move(mesh));
    }

    void MeshAsset::addPrimitive(MeshPrimitive primitive) {
        m_vertexCount += primitive.positions.size();

        if (!primitive.positions.empty()) {
            if (m_bounds.empty) {
                m_bounds.min = primitive.positions.front();
                m_bounds.max = primitive.positions.front();
                m_bounds.empty = false;
            }

            for (auto const& position : primitive.positions) {
                m_bounds.min = glm::min(m_bounds.min, position);
                m_bounds.max = glm::max(m_bounds.max, position);
            }
        }

        m_primitives.push_back(std::move(primitive));
    }

    std::size_t MeshAsset::vertexCount() const {
        return m_vertexCount;
    }

    std::size_t MeshAsset::primitiveCount() const {
        return m_primitives.size();
    }

    BoundingBox const& MeshAsset::boundingBox() const {
        return m_bounds;
    }

    std::vector<MeshPrimitive> const& MeshAsset::primitives() const {
        return m_primitives;
    }

    std::vector<MaterialData> const& MeshAsset::materials() const {
        return m_materials;
    }

    std::size_t MeshAsset::materialCount() const {
        return m_materials.size();
    }

    std::vector<ImageData> const& MeshAsset::images() const {
        return m_images;
    }

    MeshRegistry& MeshRegistry::instance() {
        static MeshRegistry registry;
        return registry;
    }

    std::uint64_t MeshRegistry::registerMesh(std::shared_ptr<MeshAsset> mesh) {
        auto const id = m_nextId++;
        m_meshes.emplace(id, std::move(mesh));
        return id;
    }

    void MeshRegistry::release(std::uint64_t id) {
        m_meshes.erase(id);
    }

    std::shared_ptr<MeshAsset> MeshRegistry::get(std::uint64_t id) const {
        auto const it = m_meshes.find(id);
        if (it == m_meshes.end()) {
            return nullptr;
        }
        return it->second;
    }

} // namespace luax::render3d
