#include "render3d/assets/MeshAsset.hpp"
#include "render3d/assets/TextureAsset.hpp"
#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/Renderer3DMeshCache.hpp"
#include "render3d/gpu/Renderer3DPrograms.hpp"
#include "render3d/gpu/SceneDrawList.hpp"
#include "render3d/gpu/VertexLayout.hpp"
#include "render3d/types/Frustum.hpp"
#include "render3d/types/SceneTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

namespace luax::render3d {
    namespace {
        glm::vec3 normalizedLightDirection(glm::vec3 const& direction) {
            if (glm::dot(direction, direction) <= 0.0f) {
                return glm::normalize(kDefaultLightDirection);
            }
            return glm::normalize(direction);
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
    } // namespace

    void runRenderer3DScenePass(
        Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache, int pixelWidth,
        int pixelHeight, Camera3D const& camera, std::map<int, ViewportInstance> const& instances,
        RenderSettings const& settings, int selfColorTexture
    ) {
        float const aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
        glm::mat4 const projection =
            glm::perspective(glm::radians(camera.fovYDegrees), aspect, camera.zNear, camera.zFar);
        glm::mat4 const view = camera.transform.inverse().toMat4();
        Frustum const frustum = Frustum::fromViewProj(projection * view);
        glm::vec3 const lightDir = normalizedLightDirection(settings.lightDirection);
        glm::vec3 const lightColor = settings.lightColor * settings.lightIntensity;

        auto resolveGpuMesh = [&](std::uint64_t meshId, MeshAsset const& mesh) -> GpuMesh* {
            return meshCache.ensureGpuMesh(meshId, mesh);
        };
        auto resolveTexture = [&](std::uint64_t textureId,
                                  TextureAsset const& texture) -> unsigned int {
            return meshCache.ensureGpuTexture(textureId, texture);
        };

        SceneDrawLists lists = buildSceneDrawLists(instances, view, frustum, resolveGpuMesh);

        bool const useVao = vaoSupported();

        auto drawSingleItem = [&](SceneDrawItem const& item, DrawPassState& state) {
            if (state.activeProgram != programs.lambertProgram) {
                glUseProgram(programs.lambertProgram);
                state.activeProgram = programs.lambertProgram;
                glUniform3fv(programs.lambertLocLightDir, 1, glm::value_ptr(lightDir));
                glUniform3fv(programs.lambertLocLightColor, 1, glm::value_ptr(lightColor));
                glUniform1f(programs.lambertLocAmbient, settings.ambient);
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
            glm::mat4 const normalMat =
                glm::mat4(glm::inverse(glm::transpose(glm::mat3(item.model))));
            glUniformMatrix4fv(programs.lambertLocMvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(programs.lambertLocNormalMat, 1, GL_FALSE, glm::value_ptr(normalMat));
            glUniform3fv(programs.lambertLocTint, 1, glm::value_ptr(item.tint));

            unsigned int const boundTexture = item.boundTexture;
            bool const useTexture = boundTexture != 0;
            float const useTextureUniform = useTexture ? 1.0f : 0.0f;
            float const shaderCutoff = shaderAlphaCutoff(item.alphaMode, item.alphaCutoff);

            if (item.baseColor != state.lastBaseColor) {
                glUniform4fv(programs.lambertLocBaseColor, 1, glm::value_ptr(item.baseColor));
                state.lastBaseColor = item.baseColor;
            }
            if (useTextureUniform != state.lastUseTexture) {
                glUniform1f(programs.lambertLocUseTexture, useTextureUniform);
                state.lastUseTexture = useTextureUniform;
            }
            if (shaderCutoff != state.lastAlphaCutoff) {
                glUniform1f(programs.lambertLocAlphaCutoff, shaderCutoff);
                state.lastAlphaCutoff = shaderCutoff;
            }
            if (useTexture && boundTexture != state.lastBoundTexture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, boundTexture);
                if (!state.textureUnitSet) {
                    glUniform1i(programs.lambertLocTexture, 0);
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

        auto drawInstancedRun = [&](SceneDrawItem const* begin,
                                    SceneDrawItem const* end,
                                    DrawPassState& state) {
            SceneDrawItem const& first = *begin;
            std::size_t const count = static_cast<std::size_t>(end - begin);

            if (state.activeProgram != programs.lambertInstProgram) {
                glUseProgram(programs.lambertInstProgram);
                state.activeProgram = programs.lambertInstProgram;
                glUniform3fv(programs.lambertInstLocLightDir, 1, glm::value_ptr(lightDir));
                glUniform3fv(programs.lambertInstLocLightColor, 1, glm::value_ptr(lightColor));
                glUniform1f(programs.lambertInstLocAmbient, settings.ambient);
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
            glUniformMatrix4fv(programs.lambertInstLocViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));

            unsigned int const boundTexture = first.boundTexture;
            bool const useTexture = boundTexture != 0;
            float const useTextureUniform = useTexture ? 1.0f : 0.0f;
            float const shaderCutoff = shaderAlphaCutoff(first.alphaMode, first.alphaCutoff);

            if (first.baseColor != state.lastBaseColor) {
                glUniform4fv(programs.lambertInstLocBaseColor, 1, glm::value_ptr(first.baseColor));
                state.lastBaseColor = first.baseColor;
            }
            if (useTextureUniform != state.lastUseTexture) {
                glUniform1f(programs.lambertInstLocUseTexture, useTextureUniform);
                state.lastUseTexture = useTextureUniform;
            }
            if (shaderCutoff != state.lastAlphaCutoff) {
                glUniform1f(programs.lambertInstLocAlphaCutoff, shaderCutoff);
                state.lastAlphaCutoff = shaderCutoff;
            }
            if (useTexture && boundTexture != state.lastBoundTexture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, boundTexture);
                if (!state.textureUnitSet) {
                    glUniform1i(programs.lambertInstLocTexture, 0);
                    state.textureUnitSet = true;
                }
                state.lastBoundTexture = boundTexture;
            }

            std::vector<GpuInstanceData> instanceData;
            instanceData.reserve(count);
            for (SceneDrawItem const* it = begin; it != end; ++it) {
                instanceData.push_back(GpuInstanceData{it->model, glm::vec4(it->tint, 0.0f)});
            }

            glBindBuffer(GL_ARRAY_BUFFER, programs.instanceVbo);
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
            setupInstanceAttribs(programs.instanceVbo);
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

        auto drawOpaqueItems = [&](std::vector<SceneDrawItem> const& items, DrawPassState& state) {
            bool const canInstance = useVao && instancingSupported() &&
                programs.ensureLambertInstProgram() && programs.ensureInstanceVbo();

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

        auto drawBlendItems = [&](std::vector<SceneDrawItem> const& items, DrawPassState& state) {
            for (auto const& item : items) {
                drawSingleItem(item, state);
            }
        };

        for (auto& item : lists.opaque) {
            item.boundTexture = resolveSceneDrawTexture(item, resolveTexture, selfColorTexture);
        }
        sortOpaqueDrawItems(lists.opaque);

        DrawPassState passState{};
        drawOpaqueItems(lists.opaque, passState);

        if (!lists.blend.empty()) {
            sortBlendDrawItems(lists.blend);
            for (auto& item : lists.blend) {
                item.boundTexture = resolveSceneDrawTexture(item, resolveTexture, selfColorTexture);
            }
            passState.lastBoundTexture = ~0u;
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            drawBlendItems(lists.blend, passState);
            glDepthMask(GL_TRUE);
        }
    }

} // namespace luax::render3d
