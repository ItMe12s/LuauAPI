#include "render3d/gpu/Renderer3D.hpp"

#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/Renderer3DMeshCache.hpp"
#include "render3d/gpu/Renderer3DPrograms.hpp"
#include "render3d/types/SceneTypes.hpp"

#include <Geode/Geode.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <map>

namespace luax::render3d {
    void destroyRenderer3DGlResources(Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache);
    void ensureRenderer3DShutdownHook();
    void runRenderer3DScenePass(
        Renderer3DPrograms& programs, Renderer3DMeshCache& meshCache, int pixelWidth,
        int pixelHeight, Camera3D const& camera, std::map<int, ViewportInstance> const& instances,
        RenderSettings const& settings, int selfColorTexture
    );
    void drawDebugOverlay(
        Renderer3DPrograms& programs, glm::mat4 const& projection, glm::mat4 const& view,
        std::map<int, DebugLine> const& debugLines, bool debugBounds,
        std::map<int, ViewportInstance> const& instances
    );

    Renderer3D& Renderer3D::instance() {
        static Renderer3D s_renderer;
        return s_renderer;
    }

    Renderer3D::~Renderer3D() {
        destroyGlResources();
    }

    void Renderer3D::syncContextGen() {
        if (m_gen != glContextGeneration()) {
            m_programs.reset();
            m_meshCache.clear();
            m_gen = glContextGeneration();
        }
    }

    void Renderer3D::destroyGlResources() {
        syncContextGen();
        destroyRenderer3DGlResources(m_programs, m_meshCache);
    }

    void Renderer3D::releaseMeshGpu(std::uint64_t meshId) {
        ensureRenderer3DShutdownHook();
        syncContextGen();
        m_meshCache.releaseMeshGpu(meshId);
    }

    void Renderer3D::releaseTextureGpu(std::uint64_t textureId) {
        ensureRenderer3DShutdownHook();
        syncContextGen();
        m_meshCache.releaseTextureGpu(textureId);
    }

    void Renderer3D::renderToFramebuffer(
        unsigned int fbo, int pixelWidth, int pixelHeight, Camera3D const& camera,
        std::map<int, ViewportInstance> const& instances, RenderSettings const& settings,
        std::map<int, DebugLine> const& debugLines, bool debugBounds
    ) {
        if (gpuFeaturesDisabled() || fbo == 0 || pixelWidth <= 0 || pixelHeight <= 0) {
            return;
        }
        ensureRenderer3DShutdownHook();
        syncContextGen();
        if (!m_programs.ensureLambertProgram()) {
            return;
        }

        DrawStateSnapshot prevState{};
        prevState.capture();

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

        runRenderer3DScenePass(
            m_programs, m_meshCache, pixelWidth, pixelHeight, camera, instances, settings, selfColorTexture
        );

        if (useVao) {
            bindVao(0);
        }
        if (!debugLines.empty() || debugBounds) {
            float const aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
            glm::mat4 const projection =
                glm::perspective(glm::radians(camera.fovYDegrees), aspect, camera.zNear, camera.zFar);
            glm::mat4 const view = camera.transform.inverse().toMat4();
            drawDebugOverlay(m_programs, projection, view, debugLines, debugBounds, instances);
        }
        if (!useVao) {
            glDisableVertexAttribArray(0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(2);
        }

        prevState.restore();
    }

} // namespace luax::render3d
