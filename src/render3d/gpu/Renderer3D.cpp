#include "render3d/gpu/Renderer3D.hpp"

#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/Renderer3DDebugOverlay.hpp"
#include "render3d/gpu/Renderer3DResourceLifetime.hpp"
#include "render3d/gpu/Renderer3DScenePass.hpp"

#include <Geode/Geode.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace luax::render3d {
    using cocos2d::ccGLEnableVertexAttribs;
    using cocos2d::ccGLUseProgram;

    Renderer3D& Renderer3D::instance() {
        static Renderer3D s_renderer;
        return s_renderer;
    }

    Renderer3D::~Renderer3D() {
        destroyGlResources();
    }

    void Renderer3D::destroyGlResources() {
        destroyRenderer3DGlResources(m_programs, m_meshCache);
    }

    void Renderer3D::releaseMeshGpu(std::uint64_t meshId) {
        ensureRenderer3DShutdownHook();
        m_meshCache.releaseMeshGpu(meshId);
    }

    void Renderer3D::releaseTextureGpu(std::uint64_t textureId) {
        ensureRenderer3DShutdownHook();
        m_meshCache.releaseTextureGpu(textureId);
    }

    void Renderer3D::renderToFramebuffer(
        unsigned int fbo, int pixelWidth, int pixelHeight, Camera3D const& camera,
        std::map<int, ViewportInstance> const& instances, RenderSettings const& settings,
        std::map<int, DebugLine> const& debugLines, bool debugBounds
    ) {
        if (fbo == 0 || pixelWidth <= 0 || pixelHeight <= 0) {
            return;
        }
        ensureRenderer3DShutdownHook();
        if (!m_programs.ensureLambertProgram()) {
            return;
        }

        float prevClearColor[4]{};
        DrawStateSnapshot prevState{};
        prevState.capture();

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

        glClearColor(prevClearColor[0], prevClearColor[1], prevClearColor[2], prevClearColor[3]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        restoreVao(prevVao);
        prevState.restore();
        ccGLUseProgram(0);
        ccGLEnableVertexAttribs(cocos2d::kCCVertexAttribFlag_None);
    }

} // namespace luax::render3d
