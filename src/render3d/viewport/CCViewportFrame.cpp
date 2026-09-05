#include "render3d/viewport/CCViewportFrame.hpp"

#include "render3d/assets/TextureAsset.hpp"
#include "render3d/gpu/GlUtil.hpp"
#include "render3d/gpu/Renderer3D.hpp"
#include "render3d/gpu/Texture2D.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_set>
#include <utility>

// Viewport FBO composite adapted from 3D-API (https://github.com/undefined06855/3D-API)
// Thank you Undefined!!!

namespace luax::render3d {
    using cocos2d::CCSize;
    using cocos2d::CCSprite;
    using cocos2d::CCTexture2D;

    namespace {
        std::pair<int, int> pixelSize(CCSize const& points) {
            float const scale = cocos2d::CCDirector::sharedDirector()->getContentScaleFactor();
            auto dim = [scale](float pts) {
                return std::max(1, static_cast<int>(std::lround(pts * scale)));
            };
            return {dim(points.width), dim(points.height)};
        }

        struct HackCCTexture2D : cocos2d::CCTexture2D {
            bool initWithGLName(GLuint name, int pixelsWide, int pixelsHigh, CCSize const& contentSize) {
                m_uName = name;
                m_tContentSize = contentSize;
                m_uPixelsWide = static_cast<unsigned int>(pixelsWide);
                m_uPixelsHigh = static_cast<unsigned int>(pixelsHigh);
                m_ePixelFormat = cocos2d::kCCTexture2DPixelFormat_RGBA8888;
                m_fMaxS = pixelsWide > 0 ? contentSize.width / static_cast<float>(pixelsWide) : 1.0f;
                m_fMaxT = pixelsHigh > 0 ? contentSize.height / static_cast<float>(pixelsHigh) : 1.0f;
                m_bHasPremultipliedAlpha = false;
                m_bHasMipmaps = false;
                setShaderProgram(
                    cocos2d::CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTexture)
                );
                return true;
            }

            void detach() {
                m_uName = 0;
            }
        };

        std::unordered_set<CCViewportFrame*>& liveViewports() {
            static std::unordered_set<CCViewportFrame*> s_viewports;
            return s_viewports;
        }

    } // namespace

    void abandonLiveViewports() {
        for (auto* viewport : liveViewports()) {
            viewport->abandonGpuResources();
        }
    }

    CCViewportFrame* CCViewportFrame::create(float width, float height) {
        if (!gpuSessionReady()) {
            return nullptr;
        }
        auto* node = new CCViewportFrame();
        if (node != nullptr && node->initWithSize(width, height)) {
            node->autorelease();
            return node;
        }
        delete node;
        return nullptr;
    }

    CCViewportFrame::CCViewportFrame() {
        liveViewports().insert(this);
    }

    CCViewportFrame::~CCViewportFrame() {
        liveViewports().erase(this);
        Renderer3D::instance().releaseTextureGpu(m_viewportTexture.get());
        m_viewportTexture.reset();
        detachSpriteTexture();
        destroyFramebuffer();
    }

    bool CCViewportFrame::initWithSize(float width, float height) {
        if (!glContextAvailable()) {
            return false;
        }
        CCSize const points = CCSizeMake(width, height);
        auto const [pw, ph] = pixelSize(points);

        CCTexture2D* tex = buildFramebufferTexture(pw, ph);
        if (tex == nullptr || !CCSprite::initWithTexture(tex)) {
            return false;
        }
        m_framebufferTexture = tex;

        setContentSize(points);
        setAnchorPoint(ccp(0.5f, 0.5f));
        ignoreAnchorPointForPosition(false);
        refreshSpriteTexture(points);
        return true;
    }

    void CCViewportFrame::setCamera3D(Camera3D const& camera) {
        m_camera = camera;
    }

    Camera3D const& CCViewportFrame::getCamera3D() const {
        return m_camera;
    }

    void CCViewportFrame::setRenderSettings(RenderSettings const& settings) {
        m_settings = settings;
    }

    RenderSettings const& CCViewportFrame::renderSettings() const {
        return m_settings;
    }

    int CCViewportFrame::addInstance(
        std::shared_ptr<MeshAsset> mesh, Transform const& transform, glm::vec3 color
    ) {
        int id = 0;
        if (!m_instanceFreeList.empty()) {
            id = m_instanceFreeList.back();
            m_instanceFreeList.pop_back();
        }
        else {
            id = m_nextInstanceId++;
        }
        ViewportInstance instance{};
        instance.mesh = std::move(mesh);
        instance.transform = transform;
        instance.color = color;
        m_instances.emplace(id, std::move(instance));
        return id;
    }

    bool CCViewportFrame::setInstanceTransform(int instanceId, Transform const& transform) {
        auto it = m_instances.find(instanceId);
        if (it == m_instances.end()) {
            return false;
        }
        it->second.transform = transform;
        return true;
    }

    bool CCViewportFrame::setInstanceMaterial(int instanceId, std::shared_ptr<Material> material) {
        auto it = m_instances.find(instanceId);
        if (it == m_instances.end()) {
            return false;
        }
        it->second.materialOverride = std::move(material);
        return true;
    }

    bool CCViewportFrame::setInstancePrimitiveMaterial(
        int instanceId, int primitiveIndex, std::shared_ptr<Material> material
    ) {
        auto it = m_instances.find(instanceId);
        if (it == m_instances.end()) {
            return false;
        }
        if (material == nullptr) {
            it->second.primitiveOverrides.erase(primitiveIndex);
        }
        else {
            it->second.primitiveOverrides[primitiveIndex] = std::move(material);
        }
        return true;
    }

    bool CCViewportFrame::setInstanceColor(int instanceId, glm::vec3 color) {
        auto it = m_instances.find(instanceId);
        if (it == m_instances.end()) {
            return false;
        }
        it->second.color = color;
        return true;
    }

    bool CCViewportFrame::removeInstance(int instanceId) {
        if (m_instances.erase(instanceId) == 0) {
            return false;
        }
        m_instanceFreeList.push_back(instanceId);
        return true;
    }

    void CCViewportFrame::clearInstances() {
        m_instances.clear();
        m_instanceFreeList.clear();
    }

    std::unordered_map<int, ViewportInstance> const& CCViewportFrame::instances() const {
        return m_instances;
    }

    unsigned int CCViewportFrame::colorTexture() const {
        return gpuHandlesValid() ? m_colorTexture : 0;
    }

    int CCViewportFrame::framebufferPixelWidth() const {
        return gpuHandlesValid() ? m_fboPixelWidth : 0;
    }

    int CCViewportFrame::framebufferPixelHeight() const {
        return gpuHandlesValid() ? m_fboPixelHeight : 0;
    }

    void CCViewportFrame::setCompositeEnabled(bool enabled) {
        m_compositeEnabled = enabled;
    }

    int CCViewportFrame::addDebugLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) {
        int id = 0;
        if (!m_debugLineFreeList.empty()) {
            id = m_debugLineFreeList.back();
            m_debugLineFreeList.pop_back();
        }
        else {
            id = m_nextDebugLineId++;
        }
        m_debugLines.emplace(id, DebugLine{from, to, color});
        return id;
    }

    bool CCViewportFrame::removeDebugLine(int lineId) {
        if (m_debugLines.erase(lineId) == 0) {
            return false;
        }
        m_debugLineFreeList.push_back(lineId);
        return true;
    }

    void CCViewportFrame::clearDebugLines() {
        m_debugLines.clear();
        m_debugLineFreeList.clear();
    }

    void CCViewportFrame::setDebugBounds(bool enabled) {
        m_debugBounds = enabled;
    }

    std::shared_ptr<TextureAsset> CCViewportFrame::viewportTextureAsset() {
        if (!gpuSessionReady()) {
            return nullptr;
        }
        if (m_viewportTexture == nullptr) {
            auto asset = std::make_shared<TextureAsset>();
            asset->setViewportSourceNode(this);
            m_viewportTexture = std::move(asset);
        }
        return m_viewportTexture;
    }

    void CCViewportFrame::draw() {
        if (!gpuSessionReady()) {
            return;
        }
        if (!isVisible()) {
            return;
        }
        CCSize const size = getContentSize();
        if (size.width <= 0.0f || size.height <= 0.0f) {
            return;
        }
        ensureFramebuffer();
        if (m_fbo == 0 || m_colorTexture == 0) {
            return;
        }

        Renderer3D::instance().renderToFramebuffer(
            m_fbo, m_fboPixelWidth, m_fboPixelHeight, m_camera, m_instances, m_settings, m_debugLines, m_debugBounds
        );

        if (m_compositeEnabled) {
            auto* live = cocos2d::CCShaderCache::sharedShaderCache()->programForKey(
                kCCShader_PositionTextureColor
            );
            if (live == nullptr) {
                return;
            }
            if (live != getShaderProgram()) {
                setShaderProgram(live);
            }

            GLuint const spProg = live->getProgram();
            GLint const mvpLoc = spProg != 0 ? glGetUniformLocation(spProg, "CC_MVPMatrix") : -1;
            if (spProg == 0 || glIsProgram(spProg) != GL_TRUE || mvpLoc < 0) {
                return;
            }

            DrawStateSnapshot compositeState{};
            compositeState.capture();
            glUseProgram(spProg);
            kmMat4 kmP, kmMV, kmMVP;
            kmGLGetMatrix(KM_GL_PROJECTION, &kmP);
            kmGLGetMatrix(KM_GL_MODELVIEW, &kmMV);
            kmMat4Multiply(&kmMVP, &kmP, &kmMV);
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, kmMVP.mat);
            CCSprite::draw();
            compositeState.restore();
        }
    }

    bool CCViewportFrame::gpuHandlesValid() const {
        return gpuSessionReady() && m_glContextGeneration == glContextGeneration();
    }

    void CCViewportFrame::refreshSpriteTexture(CCSize const& points) {
        setTextureRect(cocos2d::CCRectMake(0.0f, 0.0f, points.width, points.height));
        setFlipY(true);
    }

    CCTexture2D* CCViewportFrame::buildFramebufferTexture(int pixelWidth, int pixelHeight) {
        if (!createFramebuffer(pixelWidth, pixelHeight)) {
            return nullptr;
        }
        auto* tex = new HackCCTexture2D();
        tex->initWithGLName(
            m_colorTexture,
            pixelWidth,
            pixelHeight,
            CCSizeMake(
                static_cast<float>(pixelWidth) /
                    cocos2d::CCDirector::sharedDirector()->getContentScaleFactor(),
                static_cast<float>(pixelHeight) /
                    cocos2d::CCDirector::sharedDirector()->getContentScaleFactor()
            )
        );
        tex->autorelease();
        return tex;
    }

    void CCViewportFrame::detachSpriteTexture() {
        if (m_framebufferTexture != nullptr && getTexture() == m_framebufferTexture) {
            static_cast<HackCCTexture2D*>(m_framebufferTexture)->detach();
        }
        m_framebufferTexture = nullptr;
    }

    void CCViewportFrame::ensureFramebuffer() {
        if (!glContextAvailable()) {
            return;
        }

        if (m_glContextGeneration != glContextGeneration()) {
            detachSpriteTexture();

            m_fbo = 0;
            m_colorTexture = 0;
            m_depthRenderbuffer = 0;
            m_fboPixelWidth = 0;
            m_fboPixelHeight = 0;
            m_glContextGeneration = glContextGeneration();
        }

        CCSize const points = getContentSize();
        auto [width, height] = pixelSize(points);
        width = std::clamp(width, 1, kMaxFramebufferDimension);
        height = std::clamp(height, 1, kMaxFramebufferDimension);
        if (m_fbo != 0 && width == m_fboPixelWidth && height == m_fboPixelHeight) {
            return;
        }

        detachSpriteTexture();
        CCTexture2D* tex = buildFramebufferTexture(width, height);
        if (tex == nullptr) {
            return;
        }
        setTexture(tex);
        m_framebufferTexture = tex;
        refreshSpriteTexture(points);
    }

    bool CCViewportFrame::createFramebuffer(int width, int height) {
        width = std::clamp(width, 1, kMaxFramebufferDimension);
        height = std::clamp(height, 1, kMaxFramebufferDimension);
        if (!glContextAvailable() || width <= 0 || height <= 0) {
            return false;
        }

        GLint prevFbo = 0;
        GLint prevRenderbuffer = 0;
        GLint prevTexture = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &prevRenderbuffer);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);

        deleteColorTexture();
        if (m_fbo != 0) {
            glDeleteFramebuffers(1, &m_fbo);
        }
        if (m_depthRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &m_depthRenderbuffer);
        }
        m_fbo = 0;
        m_depthRenderbuffer = 0;

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        unsigned int const color = allocFramebufferTexture(width, height);
        if (color == 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTexture));
            glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(prevRenderbuffer));
            glDeleteFramebuffers(1, &m_fbo);
            m_fbo = 0;
            return false;
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

        glGenRenderbuffers(1, &m_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer
        );

        GLenum const status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTexture));
        glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(prevRenderbuffer));

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            geode::log::error(
                "CCViewportFrame: framebuffer incomplete (status={:#x})", static_cast<unsigned>(status)
            );
            glDeleteTextures(1, &color);
            glDeleteFramebuffers(1, &m_fbo);
            glDeleteRenderbuffers(1, &m_depthRenderbuffer);
            m_fbo = 0;
            m_depthRenderbuffer = 0;
            return false;
        }

        m_colorTexture = color;
        m_fboPixelWidth = width;
        m_fboPixelHeight = height;
        m_glContextGeneration = glContextGeneration();
        return true;
    }

    void CCViewportFrame::deleteColorTexture() {
        if (m_colorTexture != 0 && gpuHandlesValid() && glContextAvailable()) {
            glDeleteTextures(1, &m_colorTexture);
        }
        m_colorTexture = 0;
    }

    void CCViewportFrame::destroyFramebuffer() {
        deleteColorTexture();
        bool const canDelete = gpuHandlesValid() && glContextAvailable();
        if (canDelete) {
            if (m_fbo != 0) {
                glDeleteFramebuffers(1, &m_fbo);
            }
            if (m_depthRenderbuffer != 0) {
                glDeleteRenderbuffers(1, &m_depthRenderbuffer);
            }
        }
        m_fbo = 0;
        m_depthRenderbuffer = 0;
        m_fboPixelWidth = 0;
        m_fboPixelHeight = 0;
    }

    void CCViewportFrame::abandonGpuResources() {
        Renderer3D::instance().releaseTextureGpu(m_viewportTexture.get());
        m_viewportTexture.reset();
        detachSpriteTexture();
        m_fbo = 0;
        m_colorTexture = 0;
        m_depthRenderbuffer = 0;
        m_fboPixelWidth = 0;
        m_fboPixelHeight = 0;
        m_glContextGeneration = glContextGeneration();
    }

} // namespace luax::render3d
