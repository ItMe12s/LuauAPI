#include "render3d/Renderer3D.hpp"

#include "render3d/GlUtil.hpp"
#include "render3d/MeshAsset.hpp"

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
uniform vec4 uBaseColor;
uniform sampler2D uTexture;
uniform float uUseTexture;
uniform vec3 uTint;

void main() {
    vec3 albedo = uUseTexture > 0.5 ? texture2D(uTexture, vTexCoord).rgb : uBaseColor.rgb;
    albedo *= uTint;
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    float lit = 0.15 + diff * 0.85;
    gl_FragColor = vec4(albedo * lit, uBaseColor.a);
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

    } // namespace

    Renderer3D& Renderer3D::instance() {
        static Renderer3D s_renderer;
        return s_renderer;
    }

    Renderer3D::~Renderer3D() {
        destroyGlResources();
    }

    void Renderer3D::deleteGpuPrimitive(GpuPrimitive& primitive) {
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
        m_lambertLocBaseColor = glGetUniformLocation(m_lambertProgram, "uBaseColor");
        m_lambertLocTexture = glGetUniformLocation(m_lambertProgram, "uTexture");
        m_lambertLocUseTexture = glGetUniformLocation(m_lambertProgram, "uUseTexture");
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
        std::map<int, ViewportInstance> const& instances
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

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, pixelWidth, pixelHeight);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
        glClearColor(0.08f, 0.09f, 0.12f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float const aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
        glm::mat4 const projection =
            glm::perspective(glm::radians(camera.fovYDegrees), aspect, camera.zNear, camera.zFar);
        glm::mat4 const view = camera.transform.inverse().toMat4();
        glm::vec3 const lightDir = glm::normalize(glm::vec3(0.35f, 0.85f, 0.4f));

        glUseProgram(m_lambertProgram);
        glUniform3fv(m_lambertLocLightDir, 1, glm::value_ptr(lightDir));

        int const stride = static_cast<int>(sizeof(InterleavedVertex));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

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
            glm::mat4 const mvp = projection * view * model;
            glm::mat4 const normalMat = model;
            auto const& materials = instance.mesh->materials();

            glUniformMatrix4fv(m_lambertLocMvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(m_lambertLocNormalMat, 1, GL_FALSE, glm::value_ptr(normalMat));
            glUniform3fv(m_lambertLocTint, 1, glm::value_ptr(instance.color));

            for (auto const& primitive : gpuMesh->primitives) {
                if (primitive.vbo == 0 || primitive.ibo == 0 || primitive.indexCount == 0) {
                    continue;
                }

                glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
                int imageIndex = -1;
                GpuMesh const* textureSource = gpuMesh;

                if (instance.materialOverride != nullptr) {
                    Material const& overrideMat = *instance.materialOverride;
                    baseColor = overrideMat.baseColorFactor;
                    imageIndex = overrideMat.imageIndex;
                    if (imageIndex >= 0 && overrideMat.sourceMesh != nullptr) {
                        textureSource =
                            ensureGpuMesh(overrideMat.sourceMeshId, *overrideMat.sourceMesh);
                        if (textureSource == nullptr) {
                            imageIndex = -1;
                        }
                    }
                }
                else if (
                    primitive.materialIndex >= 0 &&
                    static_cast<std::size_t>(primitive.materialIndex) < materials.size()
                ) {
                    auto const& material =
                        materials[static_cast<std::size_t>(primitive.materialIndex)];
                    baseColor = material.baseColorFactor;
                    imageIndex = material.imageIndex;
                }

                bool const useTexture = imageIndex >= 0 && textureSource != nullptr &&
                    static_cast<std::size_t>(imageIndex) < textureSource->textures.size() &&
                    textureSource->textures[static_cast<std::size_t>(imageIndex)] != 0;

                glUniform4fv(m_lambertLocBaseColor, 1, glm::value_ptr(baseColor));
                glUniform1f(m_lambertLocUseTexture, useTexture ? 1.0f : 0.0f);
                if (useTexture) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(
                        GL_TEXTURE_2D, textureSource->textures[static_cast<std::size_t>(imageIndex)]
                    );
                    glUniform1i(m_lambertLocTexture, 0);
                }

                glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive.ibo);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
                glVertexAttribPointer(
                    1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float))
                );
                glVertexAttribPointer(
                    2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float))
                );
                glDrawElements(
                    GL_TRIANGLES, static_cast<GLsizei>(primitive.indexCount), GL_UNSIGNED_INT, nullptr
                );
            }
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

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

        CC_INCREMENT_GL_DRAWS(1);
    }

} // namespace luax::render3d
