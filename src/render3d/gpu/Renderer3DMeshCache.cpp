#include "render3d/gpu/Renderer3DMeshCache.hpp"

#include "render3d/assets/ImageDecode.hpp"
#include "render3d/assets/MeshAsset.hpp"
#include "render3d/assets/TextureAsset.hpp"
#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/Texture2D.hpp"
#include "render3d/gpu/VertexLayout.hpp"

#include <Geode/Geode.hpp>
#include <glm/glm.hpp>

namespace luax::render3d {
    namespace {
        bool canDeleteGpuResources(unsigned cacheGen) {
            return glContextAvailable() && cacheGen == glContextGeneration();
        }
    } // namespace

    void Renderer3DMeshCache::deleteGpuPrimitive(GpuPrimitive& primitive) {
        if (!canDeleteGpuResources(m_glContextGeneration)) {
            return;
        }
        if (primitive.vbo != 0) {
            glDeleteBuffers(1, &primitive.vbo);
        }
        if (primitive.ibo != 0) {
            glDeleteBuffers(1, &primitive.ibo);
        }
    }

    void Renderer3DMeshCache::deleteGpuMesh(GpuMesh& mesh) {
        if (!canDeleteGpuResources(m_glContextGeneration)) {
            return;
        }
        for (auto& primitive : mesh.primitives) {
            deleteGpuPrimitive(primitive);
        }
        for (unsigned int texture : mesh.textures) {
            if (texture != 0) {
                glDeleteTextures(1, &texture);
            }
        }
    }

    void Renderer3DMeshCache::destroyAllGpuResources() {
        if (!canDeleteGpuResources(m_glContextGeneration)) {
            clear();
            return;
        }
        for (auto& [mesh, gpuMesh] : m_gpuMeshes) {
            (void)mesh;
            deleteGpuMesh(gpuMesh);
        }
        for (auto& [textureAsset, texture] : m_gpuTextures) {
            (void)textureAsset;
            if (texture != 0) {
                glDeleteTextures(1, &texture);
            }
        }
        clear();
    }

    void Renderer3DMeshCache::clear() {
        m_gpuMeshes.clear();
        m_gpuTextures.clear();
        m_glContextGeneration = glContextGeneration();
    }

    void Renderer3DMeshCache::releaseMeshGpu(MeshAsset const* mesh) {
        auto it = m_gpuMeshes.find(mesh);
        if (it == m_gpuMeshes.end()) {
            return;
        }
        deleteGpuMesh(it->second);
        m_gpuMeshes.erase(it);
    }

    void Renderer3DMeshCache::releaseTextureGpu(TextureAsset const* texture) {
        auto it = m_gpuTextures.find(texture);
        if (it == m_gpuTextures.end()) {
            return;
        }
        if (canDeleteGpuResources(m_glContextGeneration) && it->second != 0) {
            glDeleteTextures(1, &it->second);
        }
        m_gpuTextures.erase(it);
    }

    unsigned int Renderer3DMeshCache::ensureGpuTexture(TextureAsset const& textureAsset) {
        if (!gpuSessionReady()) {
            return 0;
        }
        if (m_glContextGeneration != glContextGeneration()) {
            clear();
        }
        unsigned int const viewportTexture = textureAsset.viewportColorTexture();
        if (viewportTexture != 0) {
            return viewportTexture;
        }

        auto const existing = m_gpuTextures.find(&textureAsset);
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
        m_gpuTextures[&textureAsset] = texture;
        m_glContextGeneration = glContextGeneration();
        return texture;
    }

    GpuMesh* Renderer3DMeshCache::ensureGpuMesh(MeshAsset const& meshAsset) {
        if (!gpuSessionReady()) {
            return nullptr;
        }
        if (m_glContextGeneration != glContextGeneration()) {
            clear();
        }
        auto it = m_gpuMeshes.find(&meshAsset);
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

        auto& gpuMesh = m_gpuMeshes[&meshAsset];
        m_glContextGeneration = glContextGeneration();
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

            std::vector<std::uint16_t> indices;
            indices.reserve(src.indices.size());
            for (auto const index : src.indices) {
                indices.push_back(static_cast<std::uint16_t>(index));
            }

            drainGlErrors();
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
                static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint16_t)),
                indices.data(),
                GL_STATIC_DRAW
            );

            if (glGetError() != GL_NO_ERROR) {
                geode::log::error("Renderer3D: mesh VBO upload failed for primitive {}", i);
                if (gpu.vbo != 0) {
                    glDeleteBuffers(1, &gpu.vbo);
                }
                if (gpu.ibo != 0) {
                    glDeleteBuffers(1, &gpu.ibo);
                }
                gpu.vbo = 0;
                gpu.ibo = 0;
                gpu.indexCount = 0;
                continue;
            }

            gpu.indexCount = static_cast<unsigned int>(indices.size());
            gpu.materialIndex = src.materialIndex;
            bindInterleavedBuffers(gpu.vbo, gpu.ibo);
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
            m_gpuMeshes.erase(&meshAsset);
            return nullptr;
        }
        return &gpuMesh;
    }

} // namespace luax::render3d
