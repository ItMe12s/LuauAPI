#include "render3d/gpu/Renderer3D.hpp"

#include "render3d/assets/MeshAsset.hpp"
#include "render3d/assets/TextureAsset.hpp"
#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/ShaderSources.hpp"
#include "render3d/gpu/VertexLayout.hpp"
#include "render3d/types/Frustum.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace luax::render3d {
    using cocos2d::ccGLEnableVertexAttribs;
    using cocos2d::ccGLUseProgram;

    namespace {
        float shaderAlphaCutoff(int alphaMode, float alphaCutoff) {
            return alphaMode == 1 ? alphaCutoff : 0.0f;
        }

        glm::vec3 normalizedLightDirection(glm::vec3 const& direction) {
            glm::vec3 const fallback{0.35f, 0.85f, 0.4f};
            if (glm::dot(direction, direction) <= 0.0f) {
                return glm::normalize(fallback);
            }
            return glm::normalize(direction);
        }
    } // namespace

    Renderer3D& Renderer3D::instance() {
        static Renderer3D s_renderer;
        return s_renderer;
    }

    Renderer3D::~Renderer3D() {
        destroyGlResources();
    }

    void Renderer3D::deleteGpuPrimitive(GpuPrimitive& primitive) {
        deleteVao(primitive.vao);
        if (primitive.vbo != 0) {
            glDeleteBuffers(1, &primitive.vbo);
        }
        if (primitive.ibo != 0) {
            glDeleteBuffers(1, &primitive.ibo);
        }
        primitive = {};
    }

    void Renderer3D::deleteGpuMesh(GpuMesh& mesh) {
        for (auto& primitive : mesh.primitives) {
            deleteGpuPrimitive(primitive);
        }
        mesh.primitives.clear();
        if (glContextAvailable()) {
            for (unsigned int texture : mesh.textures) {
                if (texture != 0) {
                    glDeleteTextures(1, &texture);
                }
            }
        }
        mesh.textures.clear();
    }

    void Renderer3D::destroyGlResources() {
        if (glContextAvailable()) {
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
            if (m_blitVbo != 0) {
                glDeleteBuffers(1, &m_blitVbo);
            }
            if (m_blitProgram != 0) {
                glDeleteProgram(m_blitProgram);
            }
            if (m_lambertProgram != 0) {
                glDeleteProgram(m_lambertProgram);
            }
            if (m_lambertInstProgram != 0) {
                glDeleteProgram(m_lambertInstProgram);
            }
            if (m_instanceVbo != 0) {
                glDeleteBuffers(1, &m_instanceVbo);
            }
            if (m_debugLineVbo != 0) {
                glDeleteBuffers(1, &m_debugLineVbo);
            }
            if (m_debugLineProgram != 0) {
                glDeleteProgram(m_debugLineProgram);
            }
        }
        m_gpuMeshes.clear();
        m_gpuTextures.clear();
        m_blitVbo = 0;
        m_blitProgram = 0;
        m_lambertProgram = 0;
        m_lambertInstProgram = 0;
        m_instanceVbo = 0;
        m_debugLineVbo = 0;
        m_debugLineProgram = 0;
    }

    void Renderer3D::releaseMeshGpu(std::uint64_t meshId) {
        auto it = m_gpuMeshes.find(meshId);
        if (it == m_gpuMeshes.end()) {
            return;
        }
        if (glContextAvailable()) {
            deleteGpuMesh(it->second);
        }
        m_gpuMeshes.erase(it);
    }

    void Renderer3D::releaseTextureGpu(std::uint64_t textureId) {
        auto it = m_gpuTextures.find(textureId);
        if (it == m_gpuTextures.end()) {
            return;
        }
        if (glContextAvailable() && it->second != 0) {
            glDeleteTextures(1, &it->second);
        }
        m_gpuTextures.erase(it);
    }

    unsigned int Renderer3D::ensureGpuTexture(std::uint64_t textureId, TextureAsset const& textureAsset) {
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

        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            image.width,
            image.height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            image.rgba.data()
        );
        glBindTexture(GL_TEXTURE_2D, 0);
        m_gpuTextures[textureId] = texture;
        return texture;
    }

    bool Renderer3D::ensureLambertProgram() {
        if (m_lambertProgram != 0) {
            return true;
        }

        m_lambertProgram = buildProgram(
            shader_sources::kLambertVert,
            shader_sources::kLambertFrag,
            "lambert",
            {{0, "aPos"}, {1, "aNormal"}, {2, "aTexCoord"}}
        );
        if (m_lambertProgram == 0) {
            return false;
        }

        m_lambertLocMvp = glGetUniformLocation(m_lambertProgram, "uMVP");
        m_lambertLocNormalMat = glGetUniformLocation(m_lambertProgram, "uNormalMat");
        m_lambertLocLightDir = glGetUniformLocation(m_lambertProgram, "uLightDir");
        m_lambertLocLightColor = glGetUniformLocation(m_lambertProgram, "uLightColor");
        m_lambertLocAmbient = glGetUniformLocation(m_lambertProgram, "uAmbient");
        m_lambertLocBaseColor = glGetUniformLocation(m_lambertProgram, "uBaseColor");
        m_lambertLocTexture = glGetUniformLocation(m_lambertProgram, "uTexture");
        m_lambertLocUseTexture = glGetUniformLocation(m_lambertProgram, "uUseTexture");
        m_lambertLocAlphaCutoff = glGetUniformLocation(m_lambertProgram, "uAlphaCutoff");
        m_lambertLocTint = glGetUniformLocation(m_lambertProgram, "uTint");
        return true;
    }

    bool Renderer3D::ensureLambertInstProgram() {
        if (m_lambertInstProgram != 0) {
            return true;
        }

        m_lambertInstProgram = buildProgram(
            shader_sources::kLambertInstVert,
            shader_sources::kLambertFrag,
            "lambert-inst",
            {{0, "aPos"},
             {1, "aNormal"},
             {2, "aTexCoord"},
             {3, "aModel0"},
             {4, "aModel1"},
             {5, "aModel2"},
             {6, "aModel3"},
             {7, "aTint"}}
        );
        if (m_lambertInstProgram == 0) {
            return false;
        }

        m_lambertInstLocViewProj = glGetUniformLocation(m_lambertInstProgram, "uViewProj");
        m_lambertInstLocLightDir = glGetUniformLocation(m_lambertInstProgram, "uLightDir");
        m_lambertInstLocLightColor = glGetUniformLocation(m_lambertInstProgram, "uLightColor");
        m_lambertInstLocAmbient = glGetUniformLocation(m_lambertInstProgram, "uAmbient");
        m_lambertInstLocBaseColor = glGetUniformLocation(m_lambertInstProgram, "uBaseColor");
        m_lambertInstLocTexture = glGetUniformLocation(m_lambertInstProgram, "uTexture");
        m_lambertInstLocUseTexture = glGetUniformLocation(m_lambertInstProgram, "uUseTexture");
        m_lambertInstLocAlphaCutoff = glGetUniformLocation(m_lambertInstProgram, "uAlphaCutoff");
        return true;
    }

    bool Renderer3D::ensureInstanceVbo() {
        if (m_instanceVbo != 0) {
            return true;
        }
        glGenBuffers(1, &m_instanceVbo);
        return m_instanceVbo != 0;
    }

    bool Renderer3D::ensureBlitProgram() {
        if (m_blitProgram != 0) {
            return true;
        }

        m_blitProgram = buildProgram(
            shader_sources::kBlitVert,
            shader_sources::kBlitFrag,
            "blit",
            {{0, "aPos"}, {1, "aTexCoord"}}
        );
        if (m_blitProgram == 0) {
            return false;
        }

        m_blitLocMvp = glGetUniformLocation(m_blitProgram, "uMVP");
        m_blitLocTexture = glGetUniformLocation(m_blitProgram, "uTexture");
        return true;
    }

    bool Renderer3D::ensureBlitGeometry() {
        if (m_blitVbo != 0) {
            return true;
        }
        glGenBuffers(1, &m_blitVbo);
        return m_blitVbo != 0;
    }

    bool Renderer3D::ensureDebugLineProgram() {
        if (m_debugLineProgram != 0) {
            return true;
        }

        m_debugLineProgram = buildProgram(
            shader_sources::kDebugLineVert, shader_sources::kDebugLineFrag, "debug-line", {{0, "aPos"}}
        );
        if (m_debugLineProgram == 0) {
            return false;
        }

        m_debugLineLocMvp = glGetUniformLocation(m_debugLineProgram, "uMVP");
        m_debugLineLocColor = glGetUniformLocation(m_debugLineProgram, "uColor");
        return true;
    }

    bool Renderer3D::ensureDebugLineVbo() {
        if (m_debugLineVbo != 0) {
            return true;
        }
        glGenBuffers(1, &m_debugLineVbo);
        return m_debugLineVbo != 0;
    }

    void Renderer3D::drawDebugOverlay(
        glm::mat4 const& projection, glm::mat4 const& view,
        std::map<int, DebugLine> const& debugLines, bool debugBounds,
        std::map<int, ViewportInstance> const& instances
    ) {
        if (!ensureDebugLineProgram() || !ensureDebugLineVbo()) {
            return;
        }

        struct LineSegment {
            glm::vec3 from{};
            glm::vec3 to{};
            glm::vec3 color{1.0f, 1.0f, 1.0f};
        };

        std::vector<LineSegment> segments;
        segments.reserve(debugLines.size() + instances.size() * 12);

        for (auto const& [lineId, line] : debugLines) {
            (void)lineId;
            segments.push_back(LineSegment{line.from, line.to, line.color});
        }

        if (debugBounds) {
            glm::vec3 const boundsColor{0.0f, 1.0f, 0.0f};
            for (auto const& [instanceId, instance] : instances) {
                (void)instanceId;
                if (instance.mesh == nullptr) {
                    continue;
                }
                BoundingBox const& bounds = instance.mesh->boundingBox();
                if (bounds.empty) {
                    continue;
                }

                glm::mat4 const model = instance.transform.toMat4();
                glm::vec3 const corners[8] = {
                    glm::vec3(model * glm::vec4(bounds.min.x, bounds.min.y, bounds.min.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.max.x, bounds.min.y, bounds.min.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.max.x, bounds.max.y, bounds.min.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.min.x, bounds.max.y, bounds.min.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.min.x, bounds.min.y, bounds.max.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.max.x, bounds.min.y, bounds.max.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.max.x, bounds.max.y, bounds.max.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.min.x, bounds.max.y, bounds.max.z, 1.0f)),
                };

                auto addEdge = [&](int a, int b) {
                    segments.push_back(LineSegment{corners[a], corners[b], boundsColor});
                };

                addEdge(0, 1);
                addEdge(1, 2);
                addEdge(2, 3);
                addEdge(3, 0);
                addEdge(4, 5);
                addEdge(5, 6);
                addEdge(6, 7);
                addEdge(7, 4);
                addEdge(0, 4);
                addEdge(1, 5);
                addEdge(2, 6);
                addEdge(3, 7);
            }
        }

        if (segments.empty()) {
            return;
        }

        std::sort(segments.begin(), segments.end(), [](LineSegment const& a, LineSegment const& b) {
            if (a.color.x != b.color.x) {
                return a.color.x < b.color.x;
            }
            if (a.color.y != b.color.y) {
                return a.color.y < b.color.y;
            }
            return a.color.z < b.color.z;
        });

        bindVao(0);

        glm::mat4 const viewProj = projection * view;
        glUseProgram(m_debugLineProgram);
        glUniformMatrix4fv(m_debugLineLocMvp, 1, GL_FALSE, glm::value_ptr(viewProj));
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        glBindBuffer(GL_ARRAY_BUFFER, m_debugLineVbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(glm::vec3)), nullptr
        );

        std::size_t runStart = 0;
        while (runStart < segments.size()) {
            glm::vec3 const runColor = segments[runStart].color;
            std::size_t runEnd = runStart + 1;
            while (runEnd < segments.size() && segments[runEnd].color == runColor) {
                ++runEnd;
            }

            std::vector<glm::vec3> vertices;
            vertices.reserve((runEnd - runStart) * 2);
            for (std::size_t i = runStart; i < runEnd; ++i) {
                vertices.push_back(segments[i].from);
                vertices.push_back(segments[i].to);
            }

            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
                vertices.data(),
                GL_DYNAMIC_DRAW
            );
            glUniform3fv(m_debugLineLocColor, 1, glm::value_ptr(runColor));
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

            runStart = runEnd;
        }

        glDisableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDepthMask(GL_TRUE);
    }

    Renderer3D::GpuMesh* Renderer3D::ensureGpuMesh(std::uint64_t meshId, MeshAsset const& meshAsset) {
        auto& gpuMesh = m_gpuMeshes[meshId];
        if (!gpuMesh.primitives.empty()) {
            return &gpuMesh;
        }

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

            unsigned int texture = 0;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                image.width,
                image.height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                image.rgba.data()
            );
            gpuMesh.textures[i] = texture;
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        return &gpuMesh;
    }

    void Renderer3D::renderToFramebuffer(
        unsigned int fbo, int pixelWidth, int pixelHeight, Camera3D const& camera,
        std::map<int, ViewportInstance> const& instances, RenderSettings const& settings,
        std::map<int, DebugLine> const& debugLines, bool debugBounds
    ) {
        if (fbo == 0 || pixelWidth <= 0 || pixelHeight <= 0) {
            return;
        }
        if (!ensureLambertProgram()) {
            return;
        }

        int prevFbo = 0;
        int prevViewport[4]{};
        float prevClearColor[4]{};
        DrawStateSnapshot prevState{};
        prevState.capture();

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        glGetIntegerv(GL_VIEWPORT, prevViewport);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, prevClearColor);
        int const prevVao = captureAndUnbindVao();
        bool const useVao = vaoSupported();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, pixelWidth, pixelHeight);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
        glClearColor(
            settings.clearColor.r, settings.clearColor.g, settings.clearColor.b, settings.clearColor.a
        );
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int selfColorTexture = 0;
        glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &selfColorTexture
        );

        float const aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
        glm::mat4 const projection =
            glm::perspective(glm::radians(camera.fovYDegrees), aspect, camera.zNear, camera.zFar);
        glm::mat4 const view = camera.transform.inverse().toMat4();
        Frustum const frustum = Frustum::fromViewProj(projection * view);
        glm::vec3 const lightDir = normalizedLightDirection(settings.lightDirection);
        glm::vec3 const lightColor = settings.lightColor * settings.lightIntensity;

        struct DrawItem {
            GpuPrimitive const* prim = nullptr;
            GpuMesh const* texSource = nullptr;
            TextureAsset const* textureAsset = nullptr;
            std::uint64_t textureId = 0;
            int imageIndex = -1;
            unsigned int boundTexture = 0;
            glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
            glm::vec3 tint{1.0f, 1.0f, 1.0f};
            glm::mat4 model{1.0f};
            float viewDepth = 0.0f;
            int alphaMode = 0;
            float alphaCutoff = 0.5f;
            bool doubleSided = false;
        };

        auto resolveBoundTexture = [&](DrawItem const& item) -> unsigned int {
            unsigned int resolved = 0;
            if (item.textureId != 0 && item.textureAsset != nullptr) {
                resolved = ensureGpuTexture(item.textureId, *item.textureAsset);
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
        };

        std::vector<DrawItem> opaqueItems;
        std::vector<DrawItem> blendItems;
        opaqueItems.reserve(instances.size() * 4);
        blendItems.reserve(instances.size());

        for (auto const& [instanceId, instance] : instances) {
            (void)instanceId;
            if (instance.mesh == nullptr) {
                continue;
            }
            GpuMesh* gpuMesh = ensureGpuMesh(instance.meshId, *instance.mesh);
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

                DrawItem item{};
                item.prim = &primitive;
                item.tint = instance.color;
                item.model = model;
                item.viewDepth = viewDepth;
                item.texSource = gpuMesh;

                auto applyRuntimeMaterial = [&](Material const& overrideMat) {
                    item.baseColor = overrideMat.baseColorFactor;
                    item.alphaMode = overrideMat.alphaMode;
                    item.alphaCutoff = overrideMat.alphaCutoff;
                    item.doubleSided = overrideMat.doubleSided;
                    item.textureId = 0;
                    item.textureAsset = nullptr;
                    item.imageIndex = overrideMat.imageIndex;
                    if (overrideMat.textureId != 0 && overrideMat.texture != nullptr) {
                        item.textureId = overrideMat.textureId;
                        item.textureAsset = overrideMat.texture.get();
                        item.imageIndex = -1;
                    }
                    else if (item.imageIndex >= 0 && overrideMat.sourceMesh != nullptr) {
                        item.texSource =
                            ensureGpuMesh(overrideMat.sourceMeshId, *overrideMat.sourceMesh);
                        if (item.texSource == nullptr) {
                            item.imageIndex = -1;
                        }
                    }
                };

                auto const primOverrideIt =
                    instance.primitiveOverrides.find(static_cast<int>(primitiveIndex));
                if (primOverrideIt != instance.primitiveOverrides.end() &&
                    primOverrideIt->second != nullptr) {
                    applyRuntimeMaterial(*primOverrideIt->second);
                }
                else if (instance.materialOverride != nullptr) {
                    applyRuntimeMaterial(*instance.materialOverride);
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

                if (item.alphaMode == 2) {
                    blendItems.push_back(item);
                }
                else {
                    opaqueItems.push_back(item);
                }
            }
        }

        struct DrawPassState {
            bool cullEnabled = true;
            unsigned int lastBoundTexture = ~0u;
            unsigned int lastVao = ~0u;
            unsigned int lastVbo = ~0u;
            unsigned int lastIbo = ~0u;
            glm::vec4 lastBaseColor{-1.0f, -1.0f, -1.0f, -1.0f};
            float lastUseTexture = -1.0f;
            float lastAlphaCutoff = -1.0f;
            bool textureUnitSet = false;
            unsigned int activeProgram = 0;

            void invalidateUniformCache() {
                lastBaseColor = glm::vec4(-1.0f, -1.0f, -1.0f, -1.0f);
                lastUseTexture = -1.0f;
                lastAlphaCutoff = -1.0f;
                textureUnitSet = false;
            }
        };

        auto drawSingleItem = [&](DrawItem const& item, DrawPassState& state) {
            if (state.activeProgram != m_lambertProgram) {
                glUseProgram(m_lambertProgram);
                state.activeProgram = m_lambertProgram;
                glUniform3fv(m_lambertLocLightDir, 1, glm::value_ptr(lightDir));
                glUniform3fv(m_lambertLocLightColor, 1, glm::value_ptr(lightColor));
                glUniform1f(m_lambertLocAmbient, settings.ambient);
                state.invalidateUniformCache();
            }

            bool const wantCull = !item.doubleSided;
            if (wantCull != state.cullEnabled) {
                if (wantCull) {
                    glEnable(GL_CULL_FACE);
                }
                else {
                    glDisable(GL_CULL_FACE);
                }
                state.cullEnabled = wantCull;
            }

            glm::mat4 const mvp = projection * view * item.model;
            glm::mat4 const normalMat = item.model;
            glUniformMatrix4fv(m_lambertLocMvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(m_lambertLocNormalMat, 1, GL_FALSE, glm::value_ptr(normalMat));
            glUniform3fv(m_lambertLocTint, 1, glm::value_ptr(item.tint));

            unsigned int const boundTexture = item.boundTexture;
            bool const useTexture = boundTexture != 0;
            float const useTextureUniform = useTexture ? 1.0f : 0.0f;
            float const shaderCutoff = shaderAlphaCutoff(item.alphaMode, item.alphaCutoff);

            if (item.baseColor != state.lastBaseColor) {
                glUniform4fv(m_lambertLocBaseColor, 1, glm::value_ptr(item.baseColor));
                state.lastBaseColor = item.baseColor;
            }
            if (useTextureUniform != state.lastUseTexture) {
                glUniform1f(m_lambertLocUseTexture, useTextureUniform);
                state.lastUseTexture = useTextureUniform;
            }
            if (shaderCutoff != state.lastAlphaCutoff) {
                glUniform1f(m_lambertLocAlphaCutoff, shaderCutoff);
                state.lastAlphaCutoff = shaderCutoff;
            }
            if (useTexture && boundTexture != state.lastBoundTexture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, boundTexture);
                if (!state.textureUnitSet) {
                    glUniform1i(m_lambertLocTexture, 0);
                    state.textureUnitSet = true;
                }
                state.lastBoundTexture = boundTexture;
            }

            if (useVao && item.prim->vao != 0) {
                if (item.prim->vao != state.lastVao) {
                    bindVao(item.prim->vao);
                    state.lastVao = item.prim->vao;
                    state.lastVbo = ~0u;
                    state.lastIbo = ~0u;
                }
            }
            else {
                if (useVao && state.lastVao != 0) {
                    bindVao(0);
                    state.lastVao = 0;
                    state.lastVbo = ~0u;
                    state.lastIbo = ~0u;
                }
                bool const vboChanged = item.prim->vbo != state.lastVbo;
                if (vboChanged) {
                    glBindBuffer(GL_ARRAY_BUFFER, item.prim->vbo);
                    state.lastVbo = item.prim->vbo;
                }
                if (item.prim->ibo != state.lastIbo) {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, item.prim->ibo);
                    state.lastIbo = item.prim->ibo;
                }
                if (vboChanged) {
                    setupInterleavedVertexAttribs();
                }
            }
            glDrawElements(
                GL_TRIANGLES, static_cast<GLsizei>(item.prim->indexCount), GL_UNSIGNED_INT, nullptr
            );
        };

        auto drawInstancedRun = [&](DrawItem const* begin, DrawItem const* end, DrawPassState& state) {
            DrawItem const& first = *begin;
            std::size_t const count = static_cast<std::size_t>(end - begin);

            if (state.activeProgram != m_lambertInstProgram) {
                glUseProgram(m_lambertInstProgram);
                state.activeProgram = m_lambertInstProgram;
                glUniform3fv(m_lambertInstLocLightDir, 1, glm::value_ptr(lightDir));
                glUniform3fv(m_lambertInstLocLightColor, 1, glm::value_ptr(lightColor));
                glUniform1f(m_lambertInstLocAmbient, settings.ambient);
                state.invalidateUniformCache();
            }

            bool const wantCull = !first.doubleSided;
            if (wantCull != state.cullEnabled) {
                if (wantCull) {
                    glEnable(GL_CULL_FACE);
                }
                else {
                    glDisable(GL_CULL_FACE);
                }
                state.cullEnabled = wantCull;
            }

            glm::mat4 const viewProj = projection * view;
            glUniformMatrix4fv(m_lambertInstLocViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));

            unsigned int const boundTexture = first.boundTexture;
            bool const useTexture = boundTexture != 0;
            float const useTextureUniform = useTexture ? 1.0f : 0.0f;
            float const shaderCutoff = shaderAlphaCutoff(first.alphaMode, first.alphaCutoff);

            if (first.baseColor != state.lastBaseColor) {
                glUniform4fv(m_lambertInstLocBaseColor, 1, glm::value_ptr(first.baseColor));
                state.lastBaseColor = first.baseColor;
            }
            if (useTextureUniform != state.lastUseTexture) {
                glUniform1f(m_lambertInstLocUseTexture, useTextureUniform);
                state.lastUseTexture = useTextureUniform;
            }
            if (shaderCutoff != state.lastAlphaCutoff) {
                glUniform1f(m_lambertInstLocAlphaCutoff, shaderCutoff);
                state.lastAlphaCutoff = shaderCutoff;
            }
            if (useTexture && boundTexture != state.lastBoundTexture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, boundTexture);
                if (!state.textureUnitSet) {
                    glUniform1i(m_lambertInstLocTexture, 0);
                    state.textureUnitSet = true;
                }
                state.lastBoundTexture = boundTexture;
            }

            std::vector<GpuInstanceData> instanceData;
            instanceData.reserve(count);
            for (DrawItem const* it = begin; it != end; ++it) {
                instanceData.push_back(GpuInstanceData{it->model, glm::vec4(it->tint, 0.0f)});
            }

            glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(instanceData.size() * sizeof(GpuInstanceData)),
                instanceData.data(),
                GL_DYNAMIC_DRAW
            );

            if (first.prim->vao != state.lastVao) {
                bindVao(first.prim->vao);
                state.lastVao = first.prim->vao;
                state.lastVbo = ~0u;
                state.lastIbo = ~0u;
            }
            setupInstanceAttribs(m_instanceVbo);
#if defined(GLEW_VERSION)
            glDrawElementsInstanced(
                GL_TRIANGLES,
                static_cast<GLsizei>(first.prim->indexCount),
                GL_UNSIGNED_INT,
                nullptr,
                static_cast<GLsizei>(count)
            );
#endif
            resetInstanceAttribs();
        };

        auto sameInstancedBatch = [](DrawItem const& a, DrawItem const& b) {
            return a.prim == b.prim && a.boundTexture == b.boundTexture &&
                a.baseColor == b.baseColor &&
                shaderAlphaCutoff(a.alphaMode, a.alphaCutoff) ==
                shaderAlphaCutoff(b.alphaMode, b.alphaCutoff) &&
                a.doubleSided == b.doubleSided;
        };

        auto drawOpaqueItems = [&](std::vector<DrawItem> const& items, DrawPassState& state) {
            bool const canInstance = useVao && instancingSupported() &&
                ensureLambertInstProgram() && ensureInstanceVbo();

            if (!canInstance) {
                for (auto const& item : items) {
                    drawSingleItem(item, state);
                }
                return;
            }

            for (std::size_t i = 0; i < items.size();) {
                std::size_t j = i + 1;
                while (j < items.size() && sameInstancedBatch(items[i], items[j])) {
                    ++j;
                }
                if (j - i >= 2 && items[i].prim->vao != 0) {
                    drawInstancedRun(&items[i], &items[j], state);
                }
                else {
                    for (std::size_t k = i; k < j; ++k) {
                        drawSingleItem(items[k], state);
                    }
                }
                i = j;
            }
        };

        auto drawBlendItems = [&](std::vector<DrawItem> const& items, DrawPassState& state) {
            for (auto const& item : items) {
                drawSingleItem(item, state);
            }
        };

        for (auto& item : opaqueItems) {
            item.boundTexture = resolveBoundTexture(item);
        }
        std::sort(opaqueItems.begin(), opaqueItems.end(), [](DrawItem const& a, DrawItem const& b) {
            if (a.boundTexture != b.boundTexture) {
                return a.boundTexture < b.boundTexture;
            }
            return a.prim->vbo < b.prim->vbo;
        });

        DrawPassState passState{};
        drawOpaqueItems(opaqueItems, passState);

        if (!blendItems.empty()) {
            std::sort(blendItems.begin(), blendItems.end(), [](DrawItem const& a, DrawItem const& b) {
                return a.viewDepth < b.viewDepth;
            });
            for (auto& item : blendItems) {
                item.boundTexture = resolveBoundTexture(item);
            }
            passState.lastBoundTexture = ~0u;
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            drawBlendItems(blendItems, passState);
            glDepthMask(GL_TRUE);
        }

        if (useVao) {
            bindVao(0);
        }
        if (!debugLines.empty() || debugBounds) {
            drawDebugOverlay(projection, view, debugLines, debugBounds, instances);
        }
        if (!useVao) {
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(2);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
        glClearColor(prevClearColor[0], prevClearColor[1], prevClearColor[2], prevClearColor[3]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        restoreVao(prevVao);
        prevState.restore();
        ccGLUseProgram(0);
        ccGLEnableVertexAttribs(cocos2d::kCCVertexAttribFlag_None);
    }

    void Renderer3D::drawCompositeQuad(unsigned int colorTexture, float width, float height) {
        if (colorTexture == 0 || width <= 0.0f || height <= 0.0f) {
            return;
        }
        if (!ensureBlitProgram() || !ensureBlitGeometry()) {
            return;
        }

        glActiveTexture(GL_TEXTURE0);
        DrawStateSnapshot prevState{};
        prevState.capture();
        int const prevVao = captureAndUnbindVao();

        struct BlitVertex {
            float x;
            float y;
            float u;
            float v;
        };

        std::array<BlitVertex, 4> const quad = {{
            {0.0f, 0.0f, 0.0f, 0.0f},
            {width, 0.0f, 1.0f, 0.0f},
            {0.0f, height, 0.0f, 1.0f},
            {width, height, 1.0f, 1.0f},
        }};

        glBindBuffer(GL_ARRAY_BUFFER, m_blitVbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(quad.size() * sizeof(BlitVertex)),
            quad.data(),
            GL_DYNAMIC_DRAW
        );

        kmMat4 matrixP{};
        kmMat4 matrixMV{};
        kmMat4 matrixMVP{};
        kmGLGetMatrix(KM_GL_PROJECTION, &matrixP);
        kmGLGetMatrix(KM_GL_MODELVIEW, &matrixMV);
        kmMat4Multiply(&matrixMVP, &matrixP, &matrixMV);

        glUseProgram(m_blitProgram);
        glUniformMatrix4fv(m_blitLocMvp, 1, GL_FALSE, matrixMVP.mat);
        glUniform1i(m_blitLocTexture, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTexture);

        int const blitStride = static_cast<int>(sizeof(BlitVertex));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, blitStride, reinterpret_cast<void*>(0));
        glVertexAttribPointer(
            1, 2, GL_FLOAT, GL_FALSE, blitStride, reinterpret_cast<void*>(2 * sizeof(float))
        );

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        restoreVao(prevVao);
        prevState.restore();
        ccGLUseProgram(0);
        ccGLEnableVertexAttribs(cocos2d::kCCVertexAttribFlag_None);
    }

} // namespace luax::render3d
