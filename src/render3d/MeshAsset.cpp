#include "render3d/MeshAsset.hpp"

#include "core/Config.hpp"
#include "require/PathRules.hpp"
#include "require/PathSandbox.hpp"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <string>
#include <vector>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace {
    using namespace luax;
    using namespace luax::render3d;

    struct SandboxFileContext {
        std::filesystem::path sandboxRoot;
        std::string lastError;
    };

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

    LoadResult<std::filesystem::path> canonicalSandboxRoot(std::filesystem::path const& root) {
        if (root.empty()) {
            return LoadResult<std::filesystem::path>::err("sandbox root is empty");
        }

        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(root, ec);
        if (ec) {
            return LoadResult<std::filesystem::path>::err(
                "sandbox root cannot be resolved: " + ec.message()
            );
        }

        return LoadResult<std::filesystem::path>::ok(canonical);
    }

    cgltf_result sandboxFileRead(
        cgltf_memory_options const* memory, cgltf_file_options const* file, char const* path,
        cgltf_size* size, void** data
    ) {
        void* (*memoryAlloc)(void*, cgltf_size) =
            memory->alloc_func != nullptr ? memory->alloc_func : &cgltf_default_alloc;

        auto* context = static_cast<SandboxFileContext*>(file->user_data);
        if (context == nullptr) {
            return cgltf_result_invalid_options;
        }

        std::error_code ec;
        auto resolved = std::filesystem::weakly_canonical(std::filesystem::path(path), ec);
        if (ec) {
            context->lastError = "buffer path cannot be resolved: " + ec.message();
            return cgltf_result_io_error;
        }

        if (!pathInsideRootValue(resolved, context->sandboxRoot)) {
            context->lastError = "buffer path escapes sandbox root";
            return cgltf_result_io_error;
        }

        if (!std::filesystem::is_regular_file(resolved, ec)) {
            context->lastError = "buffer file not found: " + filesystemPathString(resolved);
            return cgltf_result_file_not_found;
        }

        auto fileSize = std::filesystem::file_size(resolved, ec);
        if (ec) {
            context->lastError = "buffer file cannot be read: " + filesystemPathString(resolved);
            return cgltf_result_io_error;
        }

        if (fileSize > kMaxFsReadBytes) {
            context->lastError = "buffer file exceeds maximum read size";
            return cgltf_result_io_error;
        }

        std::ifstream input(resolved, std::ios::binary);
        if (!input.good()) {
            context->lastError = "buffer file cannot be opened: " + filesystemPathString(resolved);
            return cgltf_result_io_error;
        }

        auto* buffer = static_cast<std::uint8_t*>(memoryAlloc(memory->user_data, fileSize));
        if (buffer == nullptr) {
            return cgltf_result_out_of_memory;
        }

        input.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(fileSize));
        if (!input.good()) {
            void (*memoryFree)(void*, void*) =
                memory->free_func != nullptr ? memory->free_func : &cgltf_default_free;
            memoryFree(memory->user_data, buffer);
            context->lastError = "buffer file cannot be read: " + filesystemPathString(resolved);
            return cgltf_result_io_error;
        }

        *size = fileSize;
        *data = buffer;
        return cgltf_result_success;
    }

    void sandboxFileRelease(
        cgltf_memory_options const* memory, cgltf_file_options const* file, void* data
    ) {
        void (*memoryFree)(void*, void*) =
            memory->free_func != nullptr ? memory->free_func : &cgltf_default_free;
        memoryFree(memory->user_data, data);
        (void)file;
    }

    LoadResult<std::vector<glm::vec3>> unpackVec3Attribute(
        cgltf_accessor const* accessor, char const* label
    ) {
        if (accessor == nullptr) {
            return LoadResult<std::vector<glm::vec3>>::err(
                std::string(label) + " accessor is missing"
            );
        }

        if (accessor->is_sparse) {
            return LoadResult<std::vector<glm::vec3>>::err(
                std::string(label) + " sparse accessors are not supported"
            );
        }

        if (accessorUsesMeshopt(accessor)) {
            return LoadResult<std::vector<glm::vec3>>::err(
                std::string(label) + " meshopt-compressed accessors are not supported"
            );
        }

        if (accessor->type != cgltf_type_vec3) {
            return LoadResult<std::vector<glm::vec3>>::err(
                std::string(label) + " accessor must be vec3"
            );
        }

        cgltf_size const floatCount = cgltf_accessor_unpack_floats(accessor, nullptr, 0);
        if (floatCount == 0 || floatCount % 3 != 0) {
            return LoadResult<std::vector<glm::vec3>>::err(
                std::string(label) + " accessor has no vertices"
            );
        }

        std::vector<float> floats(floatCount);
        cgltf_accessor_unpack_floats(accessor, floats.data(), floatCount);

        std::vector<glm::vec3> values(floatCount / 3);
        for (std::size_t i = 0; i < values.size(); ++i) {
            values[i] = glm::vec3(floats[i * 3], floats[i * 3 + 1], floats[i * 3 + 2]);
        }

        return LoadResult<std::vector<glm::vec3>>::ok(std::move(values));
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
        cgltf_primitive const& primitive, glm::mat4 const& worldMatrix
    ) {
        if (primitive.has_draco_mesh_compression) {
            return LoadResult<MeshPrimitive>::err("Draco compressed primitives are not supported");
        }

        if (primitive.type != cgltf_primitive_type_triangles) {
            return LoadResult<MeshPrimitive>::err("only triangle primitives are supported");
        }

        auto const* positionAccessor =
            cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0);
        auto positionsResult = unpackVec3Attribute(positionAccessor, "position");
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
            auto normalsResult = unpackVec3Attribute(normalAccessor, "normal");
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

        MeshPrimitive meshPrimitive;
        meshPrimitive.positions = std::move(positions);
        meshPrimitive.normals = std::move(normals);
        meshPrimitive.indices = std::move(indicesResult.unwrap());
        return LoadResult<MeshPrimitive>::ok(std::move(meshPrimitive));
    }

    std::optional<std::string> extractSceneMeshes(cgltf_data const* data, MeshAsset& asset) {
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
                auto primitiveResult = extractPrimitive(primitive, worldMatrix);
                if (primitiveResult.isErr()) {
                    return primitiveResult.unwrapErr();
                }

                asset.addPrimitive(std::move(primitiveResult.unwrap()));
                foundMesh = true;
            }
        }

        if (!foundMesh) {
            return "glTF file contains no mesh primitives";
        }

        return std::nullopt;
    }
} // namespace

namespace luax::render3d {

    LoadResult<std::shared_ptr<MeshAsset>> MeshAsset::loadFromFile(std::filesystem::path const& path) {
        auto contents = readScriptFile(path);
        if (contents.isErr()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(contents.unwrapErr());
        }

        auto const& data = contents.unwrap();
        std::vector<std::uint8_t> bytes(data.begin(), data.end());
        auto sandboxRoot = path.parent_path();
        return loadFromBytes(bytes, path, sandboxRoot);
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
        options.file.read = sandboxFileRead;
        options.file.release = sandboxFileRelease;
        options.file.user_data = &fileContext;

        cgltf_data* data = nullptr;
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

        auto mesh = std::make_shared<MeshAsset>();
        auto extractError = extractSceneMeshes(data, *mesh);
        cgltf_free(data);

        if (extractError.has_value()) {
            return LoadResult<std::shared_ptr<MeshAsset>>::err(*extractError);
        }

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
