#include "render3d/Renderer3D.hpp"

#include "render3d/CCViewportFrame.hpp"
#include "render3d/GlUtil.hpp"
#include "render3d/MeshAsset.hpp"
#include "render3d/TextureAsset.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace luax::render3d {
    using cocos2d::ccGLEnableVertexAttribs;
    using cocos2d::ccGLUseProgram;

    namespace {
        char const kLambertVert[] = R"(attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec2 aTexCoord;
uniform mat4 uMVP;
uniform mat4 uNormalMat;
varying vec3 vNormal;
varying vec2 vTexCoord;

void main() {
    vNormal = mat3(uNormalMat) * aNormal;
    vTexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

        char const kLambertFrag[] = R"(#ifdef GL_ES
precision mediump float;
#endif
varying vec3 vNormal;
varying vec2 vTexCoord;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uAmbient;
uniform vec4 uBaseColor;
uniform sampler2D uTexture;
uniform float uUseTexture;
uniform float uAlphaCutoff;
uniform vec3 uTint;

void main() {
    vec4 texel = uUseTexture > 0.5 ? texture2D(uTexture, vTexCoord) : vec4(1.0);
    vec3 albedo = uUseTexture > 0.5 ? texel.rgb : uBaseColor.rgb;
    albedo *= uTint;
    float alpha = uBaseColor.a * (uUseTexture > 0.5 ? texel.a : 1.0);
    if (alpha < uAlphaCutoff) {
        discard;
    }
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    vec3 lighting = vec3(uAmbient) + uLightColor * diff;
    gl_FragColor = vec4(albedo * lighting, alpha);
}
)";

        char const kBlitVert[] = R"(attribute vec2 aPos;
attribute vec2 aTexCoord;
uniform mat4 uMVP;
varying vec2 vTexCoord;

void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

        char const kBlitFrag[] = R"(#ifdef GL_ES
precision mediump float;
#endif
uniform sampler2D uTexture;
varying vec2 vTexCoord;

void main() {
    gl_FragColor = texture2D(uTexture, vTexCoord);
}
)";

        struct InterleavedVertex {
            float px;
            float py;
            float pz;
            float nx;
            float ny;
            float nz;
            float u;
            float v;
        };

        glm::vec3 normalizedLightDirection(glm::vec3 const& direction) {
            glm::vec3 const fallback{0.35f, 0.85f, 0.4f};
            if (glm::dot(direction, direction) <= 0.0f) {
                return glm::normalize(fallback);
            }
            return glm::normalize(direction);
        }

        int const kInterleavedVertexStride = static_cast<int>(sizeof(InterleavedVertex));

        void setupInterleavedVertexAttribs() {
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(
                0, 3, GL_FLOAT, GL_FALSE, kInterleavedVertexStride, reinterpret_cast<void*>(0)
            );
            glVertexAttribPointer(
                1,
                3,
                GL_FLOAT,
                GL_FALSE,
                kInterleavedVertexStride,
                reinterpret_cast<void*>(3 * sizeof(float))
            );
            glVertexAttribPointer(
                2,
                2,
                GL_FLOAT,
                GL_FALSE,
                kInterleavedVertexStride,
                reinterpret_cast<void*>(6 * sizeof(float))
            );
        }

        void uploadGpuPrimitiveVertexLayout(unsigned int& vao, unsigned int vbo, unsigned int ibo) {
            if (!vaoSupported()) {
                return;
            }

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            setupInterleavedVertexAttribs();
            glBindVertexArray(0);
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
        if (primitive.vao != 0) {
            glDeleteVertexArrays(1, &primitive.vao);
        }
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
        }
        m_gpuMeshes.clear();
        m_gpuTextures.clear();
        m_blitVbo = 0;
        m_blitProgram = 0;
        m_lambertProgram = 0;
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
        auto* viewport = textureAsset.viewportSource();
        if (viewport != nullptr) {
            return viewport->colorTexture();
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
            kLambertVert, kLambertFrag, "lambert", {{0, "aPos"}, {1, "aNormal"}, {2, "aTexCoord"}}
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

    bool Renderer3D::ensureBlitProgram() {
        if (m_blitProgram != 0) {
            return true;
        }

        m_blitProgram = buildProgram(kBlitVert, kBlitFrag, "blit", {{0, "aPos"}, {1, "aTexCoord"}});
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
        std::map<int, ViewportInstance> const& instances, RenderSettings const& settings
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

        float const aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
        glm::mat4 const projection =
            glm::perspective(glm::radians(camera.fovYDegrees), aspect, camera.zNear, camera.zFar);
        glm::mat4 const view = camera.transform.inverse().toMat4();
        glm::vec3 const lightDir = normalizedLightDirection(settings.lightDirection);
        glm::vec3 const lightColor = settings.lightColor * settings.lightIntensity;

        glUseProgram(m_lambertProgram);
        glUniform3fv(m_lambertLocLightDir, 1, glm::value_ptr(lightDir));
        glUniform3fv(m_lambertLocLightColor, 1, glm::value_ptr(lightColor));
        glUniform1f(m_lambertLocAmbient, settings.ambient);

        if (!useVao) {
            setupInterleavedVertexAttribs();
        }

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
            if (item.textureId != 0 && item.textureAsset != nullptr) {
                return ensureGpuTexture(item.textureId, *item.textureAsset);
            }
            if (item.imageIndex >= 0 && item.texSource != nullptr &&
                static_cast<std::size_t>(item.imageIndex) < item.texSource->textures.size()) {
                return item.texSource->textures[static_cast<std::size_t>(item.imageIndex)];
            }
            return 0;
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

        auto drawItems = [&](std::vector<DrawItem> const& items) {
            bool cullEnabled = true;
            unsigned int lastBoundTexture = ~0u;
            unsigned int lastVao = ~0u;
            unsigned int lastVbo = ~0u;
            unsigned int lastIbo = ~0u;
            glm::vec4 lastBaseColor{-1.0f, -1.0f, -1.0f, -1.0f};
            float lastUseTexture = -1.0f;
            float lastAlphaCutoff = -1.0f;
            bool textureUnitSet = false;

            for (auto const& item : items) {
                bool const wantCull = !item.doubleSided;
                if (wantCull != cullEnabled) {
                    if (wantCull) {
                        glEnable(GL_CULL_FACE);
                    }
                    else {
                        glDisable(GL_CULL_FACE);
                    }
                    cullEnabled = wantCull;
                }

                glm::mat4 const mvp = projection * view * item.model;
                glm::mat4 const normalMat = item.model;
                glUniformMatrix4fv(m_lambertLocMvp, 1, GL_FALSE, glm::value_ptr(mvp));
                glUniformMatrix4fv(m_lambertLocNormalMat, 1, GL_FALSE, glm::value_ptr(normalMat));
                glUniform3fv(m_lambertLocTint, 1, glm::value_ptr(item.tint));

                unsigned int const boundTexture = item.boundTexture;
                bool const useTexture = boundTexture != 0;
                float const useTextureUniform = useTexture ? 1.0f : 0.0f;
                float const shaderCutoff = item.alphaMode == 1 ? item.alphaCutoff : 0.0f;

                if (item.baseColor != lastBaseColor) {
                    glUniform4fv(m_lambertLocBaseColor, 1, glm::value_ptr(item.baseColor));
                    lastBaseColor = item.baseColor;
                }
                if (useTextureUniform != lastUseTexture) {
                    glUniform1f(m_lambertLocUseTexture, useTextureUniform);
                    lastUseTexture = useTextureUniform;
                }
                if (shaderCutoff != lastAlphaCutoff) {
                    glUniform1f(m_lambertLocAlphaCutoff, shaderCutoff);
                    lastAlphaCutoff = shaderCutoff;
                }
                if (useTexture && boundTexture != lastBoundTexture) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, boundTexture);
                    if (!textureUnitSet) {
                        glUniform1i(m_lambertLocTexture, 0);
                        textureUnitSet = true;
                    }
                    lastBoundTexture = boundTexture;
                }

                if (useVao && item.prim->vao != 0) {
                    if (item.prim->vao != lastVao) {
                        glBindVertexArray(item.prim->vao);
                        lastVao = item.prim->vao;
                        lastVbo = ~0u;
                        lastIbo = ~0u;
                    }
                }
                else {
                    bool const vboChanged = item.prim->vbo != lastVbo;
                    if (vboChanged) {
                        glBindBuffer(GL_ARRAY_BUFFER, item.prim->vbo);
                        lastVbo = item.prim->vbo;
                    }
                    if (item.prim->ibo != lastIbo) {
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, item.prim->ibo);
                        lastIbo = item.prim->ibo;
                    }
                    if (vboChanged) {
                        setupInterleavedVertexAttribs();
                    }
                    lastVao = ~0u;
                }
                glDrawElements(
                    GL_TRIANGLES, static_cast<GLsizei>(item.prim->indexCount), GL_UNSIGNED_INT, nullptr
                );
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

        drawItems(opaqueItems);

        if (!blendItems.empty()) {
            std::sort(blendItems.begin(), blendItems.end(), [](DrawItem const& a, DrawItem const& b) {
                return a.viewDepth < b.viewDepth;
            });
            for (auto& item : blendItems) {
                item.boundTexture = resolveBoundTexture(item);
            }
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            drawItems(blendItems);
            glDepthMask(GL_TRUE);
        }

        if (useVao) {
            glBindVertexArray(0);
        }
        else {
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
