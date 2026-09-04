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
#include <unordered_map>
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

        void bindMaterial(SceneDrawItem const& item, DrawPassState& state, LambertLocs const& locs) {
            unsigned int const boundTexture = item.boundTexture;
            float const useTextureUniform = boundTexture != 0 ? 1.0f : 0.0f;
            float const shaderCutoff = shaderAlphaCutoff(item.alphaMode, item.alphaCutoff);

            if (item.baseColor != state.lastBaseColor) {
                glUniform4fv(locs.baseColor, 1, glm::value_ptr(item.baseColor));
                state.lastBaseColor = item.baseColor;
            }
            if (useTextureUniform != state.lastUseTexture) {
                glUniform1f(locs.useTexture, useTextureUniform);
                state.lastUseTexture = useTextureUniform;
            }
            if (shaderCutoff != state.lastAlphaCutoff) {
                glUniform1f(locs.alphaCutoff, shaderCutoff);
                state.lastAlphaCutoff = shaderCutoff;
            }
            if (boundTexture != 0 && boundTexture != state.lastBoundTexture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, boundTexture);
                if (!state.textureUnitSet) {
                    glUniform1i(locs.texture, 0);
                    state.textureUnitSet = true;
                }
                state.lastBoundTexture = boundTexture;
            }
        }
    } // namespace

    void runRenderer3DScenePass(
        Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache, int pixelWidth, int pixelHeight,
        Camera3D const& camera, std::unordered_map<int, ViewportInstance> const& instances,
        RenderSettings const& settings, int selfColorTexture
    ) {
        float const aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
        glm::mat4 const projection =
            glm::perspective(glm::radians(camera.fovYDegrees), aspect, camera.zNear, camera.zFar);
        glm::mat4 const view = camera.transform.inverse().toMat4();
        glm::mat4 const viewProj = projection * view;
        Frustum const frustum = Frustum::fromViewProj(viewProj);
        glm::vec3 const lightDir = normalizedLightDirection(settings.lightDirection);
        glm::vec3 const lightColor = settings.lightColor * settings.lightIntensity;

        GpuMeshResolver resolveGpuMesh = [&](MeshAsset const& mesh) -> GpuMesh* {
            return meshCache.ensureGpuMesh(mesh);
        };
        TextureResolver resolveTexture = [&](TextureAsset const& texture) -> unsigned int {
            return meshCache.ensureGpuTexture(texture);
        };

        SceneDrawLists lists = buildSceneDrawLists(instances, view, frustum, resolveGpuMesh);

        auto drawSingleItem = [&](SceneDrawItem const& item, DrawPassState& state) {
            if (state.activeProgram != programs.lambert.id) {
                glUseProgram(programs.lambert.id);
                state.activeProgram = programs.lambert.id;
                glUniform3fv(programs.lambertLocs.lightDir, 1, glm::value_ptr(lightDir));
                glUniform3fv(programs.lambertLocs.lightColor, 1, glm::value_ptr(lightColor));
                glUniform1f(programs.lambertLocs.ambient, settings.ambient);
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

            glm::mat4 const mvp = viewProj * item.model;
            glUniformMatrix4fv(programs.lambertLocs.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(programs.lambertLocs.normalMat, 1, GL_FALSE, glm::value_ptr(item.model));
            glUniform3fv(programs.lambertLocs.tint, 1, glm::value_ptr(item.tint));

            bindMaterial(item, state, programs.lambertLocs);

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
            glDrawElements(
                GL_TRIANGLES, static_cast<GLsizei>(item.prim->indexCount), GL_UNSIGNED_SHORT, nullptr
            );
        };

        for (auto& item : lists.opaque) {
            item.boundTexture = resolveSceneDrawTexture(item, resolveTexture, selfColorTexture);
        }
        sortOpaqueDrawItems(lists.opaque);

        DrawPassState passState{};
        for (auto const& item : lists.opaque) {
            drawSingleItem(item, passState);
        }

        if (!lists.blend.empty()) {
            sortBlendDrawItems(lists.blend);
            for (auto& item : lists.blend) {
                item.boundTexture = resolveSceneDrawTexture(item, resolveTexture, selfColorTexture);
            }
            passState.lastBoundTexture = ~0u;
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            for (auto const& item : lists.blend) {
                drawSingleItem(item, passState);
            }
            glDepthMask(GL_TRUE);
        }
    }

} // namespace luax::render3d
