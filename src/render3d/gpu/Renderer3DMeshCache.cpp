#include "render3d/gpu/Renderer3DMeshCache.hpp"

#include "render3d/assets/MeshAsset.hpp"
#include "render3d/assets/TextureAsset.hpp"
#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/Texture2D.hpp"
#include "render3d/gpu/VertexLayout.hpp"

#include <glm/glm.hpp>

namespace luax::render3d {
    namespace {
        bool canDeleteGpuResources(unsigned cacheGen) {
            return glContextAvailable() && cacheGen == glContextGeneration();
        }
    } // namespace

    void Renderer3DMeshCache::deleteGpuPrimitive(GpuPrimitive& primitive) {
        if (!canDeleteGpuResources(m_gen)) {
            primitive = {};
            return;
        }
        deleteVao(primitive.vao);
        if (primitive.vbo != 0) {
            glDeleteBuffers(1, &primitive.vbo);
        }
        if (primitive.ibo != 0) {
            glDeleteBuffers(1, &primitive.ibo);
        }
        primitive = {};
    }

    void Renderer3DMeshCache::deleteGpuMesh(GpuMesh& mesh) {
        if (!canDeleteGpuResources(m_gen)) {
            mesh.primitives.clear();
            mesh.textures.clear();
            return;
        }
        for (auto& primitive : mesh.primitives) {
            deleteGpuPrimitive(primitive);
        }
        mesh.primitives.clear();
        for (unsigned int texture : mesh.textures) {
            if (texture != 0) {
                glDeleteTextures(1, &texture);
            }
        }
        mesh.textures.clear();
    }

    void Renderer3DMeshCache::destroyAllGpuResources() {
        if (!canDeleteGpuResources(m_gen)) {
            clear();
            return;
        }
        for (auto& [meshId, mesh] : m_gpuMeshes) {
            (void)meshId;
            deleteGpuMesh(mesh);
        }
        for (auto& [textureId, texture] : m_gpuTextures) {
            (void)textureId;
            if (texture != 0) {
                glDeleteTextures(1, &texture);
            }
        }
        clear();
    }

    void Renderer3DMeshCache::clear() {
        m_gpuMeshes.clear();
        m_gpuTextures.clear();
        m_gen = glContextGeneration();
    }

    void Renderer3DMeshCache::releaseMeshGpu(std::uint64_t meshId) {
        auto it = m_gpuMeshes.find(meshId);
        if (it == m_gpuMeshes.end()) {
            return;
        }
        deleteGpuMesh(it->second);
        m_gpuMeshes.erase(it);
    }

    void Renderer3DMeshCache::releaseTextureGpu(std::uint64_t textureId) {
        auto it = m_gpuTextures.find(textureId);
        if (it == m_gpuTextures.end()) {
            return;
        }
        if (canDeleteGpuResources(m_gen) && it->second != 0) {
            glDeleteTextures(1, &it->second);
        }
        m_gpuTextures.erase(it);
    }

    unsigned int Renderer3DMeshCache::ensureGpuTexture(
        std::uint64_t textureId, TextureAsset const& textureAsset
    ) {
        if (!gameTexturesLoaded()) {
            return 0;
        }
        if (m_gen != glContextGeneration()) {
            clear();
        }
        unsigned int const viewportTexture = textureAsset.viewportColorTexture();
        if (viewportTexture != 0) {
            return viewportTexture;
        }

        auto const existing = m_gpuTextures.find(textureId);
        if (existing != m_gpuTextures.end() && existing->second != 0) {
            return existing->second;
        }

        auto const& image = textureAsset.cpu;
        if (image.width <= 0 || image.height <= 0 || image.rgba.empty()) {
            return 0;
        }
        if (!glContextAvailable()) {
            return 0;
        }

        unsigned int const texture = uploadRgbaTexture2D(image);
        if (texture == 0) {
            return 0;
        }
        m_gpuTextures[textureId] = texture;
        m_gen = glContextGeneration();
        return texture;
    }

    GpuMesh* Renderer3DMeshCache::ensureGpuMesh(std::uint64_t meshId, MeshAsset const& meshAsset) {
        if (!gameTexturesLoaded()) {
            return nullptr;
        }
        if (m_gen != glContextGeneration()) {
            clear();
        }
        auto it = m_gpuMeshes.find(meshId);
        if (it != m_gpuMeshes.end()) {
            if (hasDrawableGpuPrimitive(it->second)) {
                return &it->second;
            }
            deleteGpuMesh(it->second);
            m_gpuMeshes.erase(it);
        }

        if (!glContextAvailable()) {
            return nullptr;
        }

        auto& gpuMesh = m_gpuMeshes[meshId];
        m_gen = glContextGeneration();
        auto const& srcPrimitives = meshAsset.primitives();
        gpuMesh.primitives.resize(srcPrimitives.size());

        for (std::size_t i = 0; i < srcPrimitives.size(); ++i) {
            auto const& src = srcPrimitives[i];
            auto& gpu = gpuMesh.primitives[i];

            if (src.positions.empty() || src.indices.empty()) {
                continue;
            }

            std::vector<InterleavedVertex> vertices;
            vertices.reserve(src.positions.size());
            for (std::size_t v = 0; v < src.positions.size(); ++v) {
                glm::vec3 const& pos = src.positions[v];
                glm::vec3 normal{0.0f, 1.0f, 0.0f};
                if (v < src.normals.size()) {
                    normal = src.normals[v];
                }
                glm::vec2 uv{0.0f, 0.0f};
                if (v < src.texcoords.size()) {
                    uv = src.texcoords[v];
                }
                vertices.push_back(
                    InterleavedVertex{pos.x, pos.y, pos.z, normal.x, normal.y, normal.z, uv.x, uv.y}
                );
            }

            glGenBuffers(1, &gpu.vbo);
            glGenBuffers(1, &gpu.ibo);

            glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(vertices.size() * sizeof(InterleavedVertex)),
                vertices.data(),
                GL_STATIC_DRAW
            );

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ibo);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(src.indices.size() * sizeof(std::uint32_t)),
                src.indices.data(),
                GL_STATIC_DRAW
            );

            gpu.indexCount = static_cast<unsigned int>(src.indices.size());
            gpu.materialIndex = src.materialIndex;
            uploadGpuPrimitiveVertexLayout(gpu.vao, gpu.vbo, gpu.ibo);
        }

        auto const& images = meshAsset.images();
        gpuMesh.textures.resize(images.size());
        for (std::size_t i = 0; i < images.size(); ++i) {
            auto const& image = images[i];
            if (image.width <= 0 || image.height <= 0 || image.rgba.empty()) {
                gpuMesh.textures[i] = 0;
                continue;
            }

            gpuMesh.textures[i] = uploadRgbaTexture2D(image);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (!hasDrawableGpuPrimitive(gpuMesh)) {
            deleteGpuMesh(gpuMesh);
            m_gpuMeshes.erase(meshId);
            return nullptr;
        }
        return &gpuMesh;
    }

} // namespace luax::render3d
