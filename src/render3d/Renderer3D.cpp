#include "render3d/Renderer3D.hpp"

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
    using cocos2d::ccGLInvalidateStateCache;

    namespace {
        char const kLambertVert[] = R"(attribute vec3 aPos;
attribute vec3 aNormal;
uniform mat4 uMVP;
uniform mat4 uNormalMat;
varying vec3 vNormal;

void main() {
    vNormal = mat3(uNormalMat) * aNormal;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

        char const kLambertFrag[] = R"(#ifdef GL_ES
precision mediump float;
#endif
varying vec3 vNormal;
uniform vec3 uLightDir;
uniform vec3 uColor;

void main() {
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    float lit = 0.15 + diff * 0.85;
    gl_FragColor = vec4(uColor * lit, 1.0);
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
        };

        struct DrawStateSnapshot {
            GLboolean depthEnabled = GL_FALSE;
            GLboolean cullEnabled = GL_FALSE;
            GLboolean blendEnabled = GL_FALSE;
            GLint blendSrc = GL_ONE;
            GLint blendDst = GL_ZERO;
            int boundTexture = 0;

            void capture() {
                glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
                glGetBooleanv(GL_CULL_FACE, &cullEnabled);
                glGetBooleanv(GL_BLEND, &blendEnabled);
                glGetIntegerv(GL_BLEND_SRC, &blendSrc);
                glGetIntegerv(GL_BLEND_DST, &blendDst);
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
            }

            void restore() const {
                if (depthEnabled == GL_TRUE) {
                    glEnable(GL_DEPTH_TEST);
                }
                else {
                    glDisable(GL_DEPTH_TEST);
                }
                if (cullEnabled == GL_TRUE) {
                    glEnable(GL_CULL_FACE);
                }
                else {
                    glDisable(GL_CULL_FACE);
                }
                if (blendEnabled == GL_TRUE) {
                    glEnable(GL_BLEND);
                    glBlendFunc(static_cast<GLenum>(blendSrc), static_cast<GLenum>(blendDst));
                }
                else {
                    glDisable(GL_BLEND);
                }
                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(boundTexture));
            }
        };

        unsigned int compileShader(unsigned int type, char const* source) {
            unsigned int const shader = glCreateShader(type);
            char const* src = source;
            glShaderSource(shader, 1, &src, nullptr);
            glCompileShader(shader);

            int ok = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            if (ok == GL_TRUE) {
                return shader;
            }

            char log[512]{};
            glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof log), nullptr, log);
            geode::log::error("Renderer3D: shader compile failed: {}", log);
            glDeleteShader(shader);
            return 0;
        }

        unsigned int buildProgram(
            char const* vertSource, char const* fragSource, char const* label,
            std::initializer_list<std::pair<unsigned int, char const*>> attribs
        ) {
            unsigned int const vert = compileShader(GL_VERTEX_SHADER, vertSource);
            if (vert == 0) {
                return 0;
            }

            unsigned int const frag = compileShader(GL_FRAGMENT_SHADER, fragSource);
            if (frag == 0) {
                glDeleteShader(vert);
                return 0;
            }

            unsigned int program = glCreateProgram();
            for (auto const& [location, name] : attribs) {
                glBindAttribLocation(program, static_cast<GLuint>(location), name);
            }

            glAttachShader(program, vert);
            glAttachShader(program, frag);
            glLinkProgram(program);
            glDeleteShader(vert);
            glDeleteShader(frag);

            int ok = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &ok);
            if (ok != GL_TRUE) {
                char log[512]{};
                glGetProgramInfoLog(program, static_cast<GLsizei>(sizeof log), nullptr, log);
                geode::log::error("Renderer3D: {} program link failed: {}", label, log);
                glDeleteProgram(program);
                return 0;
            }

            return program;
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
    }

    void Renderer3D::destroyGlResources() {
        for (auto& [meshId, mesh] : m_gpuMeshes) {
            (void)meshId;
            deleteGpuMesh(mesh);
        }
        m_gpuMeshes.clear();

        if (m_blitVbo != 0) {
            glDeleteBuffers(1, &m_blitVbo);
            m_blitVbo = 0;
        }
        if (m_blitProgram != 0) {
            glDeleteProgram(m_blitProgram);
            m_blitProgram = 0;
        }
        if (m_lambertProgram != 0) {
            glDeleteProgram(m_lambertProgram);
            m_lambertProgram = 0;
        }
    }

    void Renderer3D::releaseMeshGpu(std::uint64_t meshId) {
        auto it = m_gpuMeshes.find(meshId);
        if (it == m_gpuMeshes.end()) {
            return;
        }
        deleteGpuMesh(it->second);
        m_gpuMeshes.erase(it);
    }

    bool Renderer3D::ensureLambertProgram() {
        if (m_lambertProgram != 0) {
            return true;
        }

        m_lambertProgram =
            buildProgram(kLambertVert, kLambertFrag, "lambert", {{0, "aPos"}, {1, "aNormal"}});
        if (m_lambertProgram == 0) {
            return false;
        }

        m_lambertLocMvp = glGetUniformLocation(m_lambertProgram, "uMVP");
        m_lambertLocNormalMat = glGetUniformLocation(m_lambertProgram, "uNormalMat");
        m_lambertLocLightDir = glGetUniformLocation(m_lambertProgram, "uLightDir");
        m_lambertLocColor = glGetUniformLocation(m_lambertProgram, "uColor");
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

    Renderer3D::GpuMesh* Renderer3D::ensureGpuMesh(std::uint64_t meshId) {
        auto meshAsset = MeshRegistry::instance().get(meshId);
        if (meshAsset == nullptr) {
            return nullptr;
        }

        auto& gpuMesh = m_gpuMeshes[meshId];
        if (!gpuMesh.primitives.empty()) {
            return &gpuMesh;
        }

        auto const& srcPrimitives = meshAsset->primitives();
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
                vertices.push_back(
                    InterleavedVertex{pos.x, pos.y, pos.z, normal.x, normal.y, normal.z}
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
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
        GLboolean depthEnabled = GL_FALSE;
        GLboolean cullEnabled = GL_FALSE;
        GLboolean blendEnabled = GL_FALSE;

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        glGetIntegerv(GL_VIEWPORT, prevViewport);
        glGetBooleanv(GL_DEPTH_TEST, &depthEnabled);
        glGetBooleanv(GL_CULL_FACE, &cullEnabled);
        glGetBooleanv(GL_BLEND, &blendEnabled);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, pixelWidth, pixelHeight);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
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

        for (auto const& [instanceId, instance] : instances) {
            (void)instanceId;
            GpuMesh* gpuMesh = ensureGpuMesh(instance.meshId);
            if (gpuMesh == nullptr) {
                continue;
            }

            glm::mat4 const model = instance.transform.toMat4();
            glm::mat4 const mvp = projection * view * model;
            glm::mat4 const normalMat = model;

            glUniformMatrix4fv(m_lambertLocMvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(m_lambertLocNormalMat, 1, GL_FALSE, glm::value_ptr(normalMat));
            glUniform3fv(m_lambertLocColor, 1, glm::value_ptr(instance.color));

            for (auto const& primitive : gpuMesh->primitives) {
                if (primitive.vbo == 0 || primitive.ibo == 0 || primitive.indexCount == 0) {
                    continue;
                }

                glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive.ibo);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
                glVertexAttribPointer(
                    1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float))
                );
                glDrawElements(
                    GL_TRIANGLES, static_cast<GLsizei>(primitive.indexCount), GL_UNSIGNED_INT, nullptr
                );
            }
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
        glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
        if (depthEnabled == GL_FALSE) {
            glDisable(GL_DEPTH_TEST);
        }
        if (cullEnabled == GL_FALSE) {
            glDisable(GL_CULL_FACE);
        }
        if (blendEnabled == GL_TRUE) {
            glEnable(GL_BLEND);
        }
        else {
            glDisable(GL_BLEND);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);
        ccGLInvalidateStateCache();
    }

    void Renderer3D::drawCompositeQuad(unsigned int colorTexture, float width, float height) {
        if (colorTexture == 0 || width <= 0.0f || height <= 0.0f) {
            return;
        }
        if (!ensureBlitProgram() || !ensureBlitGeometry()) {
            return;
        }

        DrawStateSnapshot prevState{};
        prevState.capture();

        struct BlitVertex {
            float x;
            float y;
            float u;
            float v;
        };

        std::array<BlitVertex, 4> const quad = {{
            {0.0f, 0.0f, 0.0f, 1.0f},
            {width, 0.0f, 1.0f, 1.0f},
            {0.0f, height, 0.0f, 0.0f},
            {width, height, 1.0f, 0.0f},
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
        glUseProgram(0);
        prevState.restore();
        ccGLInvalidateStateCache();

        CC_INCREMENT_GL_DRAWS(1);
    }

} // namespace luax::render3d
