#include "render3d/CCViewportFrame.hpp"

#include "render3d/GlUtil.hpp"
#include "render3d/Renderer3D.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>

namespace luax::render3d {
    using cocos2d::CCNode;
    using cocos2d::CCSize;

    namespace {
        int pixelDimension(float points, float scale) {
            return std::max(1, static_cast<int>(std::lround(points * scale)));
        }
    } // namespace

    CCViewportFrame* CCViewportFrame::create(float width, float height) {
        auto* node = new CCViewportFrame();
        if (node != nullptr && node->initWithSize(width, height)) {
            node->autorelease();
            return node;
        }
        delete node;
        return nullptr;
    }

    CCViewportFrame::CCViewportFrame() = default;

    CCViewportFrame::~CCViewportFrame() {
        destroyFramebuffer();
    }

    bool CCViewportFrame::initWithSize(float width, float height) {
        if (!CCNode::init()) {
            return false;
        }
        setAnchorPoint(ccp(0.5f, 0.5f));
        ignoreAnchorPointForPosition(false);
        setContentSize(CCSizeMake(width, height));
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
        std::uint64_t meshId, std::shared_ptr<MeshAsset> mesh, Transform const& transform,
        glm::vec3 color
    ) {
        int const id = m_nextInstanceId++;
        m_instances.emplace(id, ViewportInstance{meshId, std::move(mesh), transform, color});
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
        return m_instances.erase(instanceId) > 0;
    }

    void CCViewportFrame::clearInstances() {
        m_instances.clear();
    }

    std::map<int, ViewportInstance> const& CCViewportFrame::instances() const {
        return m_instances;
    }

    unsigned int CCViewportFrame::framebuffer() const {
        return m_fbo;
    }

    unsigned int CCViewportFrame::colorTexture() const {
        return m_colorTexture;
    }

    int CCViewportFrame::framebufferPixelWidth() const {
        return m_fboPixelWidth;
    }

    int CCViewportFrame::framebufferPixelHeight() const {
        return m_fboPixelHeight;
    }

    void CCViewportFrame::setContentSize(CCSize const& size) {
        CCNode::setContentSize(size);
        invalidateFramebuffer();
    }

    void CCViewportFrame::draw() {
        if (!isVisible()) {
            return;
        }

        CCSize const& size = getContentSize();
        if (size.width <= 0.0f || size.height <= 0.0f) {
            return;
        }
        ensureFramebuffer();
        if (m_fbo == 0 || m_colorTexture == 0) {
            return;
        }

        auto& renderer = Renderer3D::instance();
        renderer.renderToFramebuffer(
            m_fbo, m_fboPixelWidth, m_fboPixelHeight, m_camera, m_instances, m_settings
        );
        renderer.drawCompositeQuad(m_colorTexture, size.width, size.height);
    }

    bool CCViewportFrame::hasGlContext() const {
        return glContextAvailable();
    }

    void CCViewportFrame::invalidateFramebuffer() {
        destroyFramebuffer();
    }

    void CCViewportFrame::ensureFramebuffer() {
        if (!hasGlContext()) {
            return;
        }

        float const scale = cocos2d::CCDirector::sharedDirector()->getContentScaleFactor();
        int const width = pixelDimension(getContentSize().width, scale);
        int const height = pixelDimension(getContentSize().height, scale);
        if (m_fbo != 0 && width == m_fboPixelWidth && height == m_fboPixelHeight) {
            return;
        }

        destroyFramebuffer();

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        glGenTextures(1, &m_colorTexture);
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

        glGenRenderbuffers(1, &m_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer
        );

        GLenum const status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            geode::log::error(
                "CCViewportFrame: framebuffer incomplete (status={:#x})", static_cast<unsigned>(status)
            );
            destroyFramebuffer();
            return;
        }

        m_fboPixelWidth = width;
        m_fboPixelHeight = height;
    }

    void CCViewportFrame::destroyFramebuffer() {
        if (m_fbo == 0 && m_colorTexture == 0 && m_depthRenderbuffer == 0) {
            m_fboPixelWidth = 0;
            m_fboPixelHeight = 0;
            return;
        }

        if (hasGlContext()) {
            if (m_fbo != 0) {
                glDeleteFramebuffers(1, &m_fbo);
            }
            if (m_colorTexture != 0) {
                glDeleteTextures(1, &m_colorTexture);
            }
            if (m_depthRenderbuffer != 0) {
                glDeleteRenderbuffers(1, &m_depthRenderbuffer);
            }
        }

        m_fbo = 0;
        m_colorTexture = 0;
        m_depthRenderbuffer = 0;
        m_fboPixelWidth = 0;
        m_fboPixelHeight = 0;
    }

} // namespace luax::render3d
