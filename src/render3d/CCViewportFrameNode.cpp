#include "render3d/CCViewportFrameNode.hpp"

#include "render3d/Renderer3D.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>

namespace luax::render3d {
    using cocos2d::CCDirector;
    using cocos2d::CCNode;
    using cocos2d::CCSize;

    namespace {
        int pixelDimension(float points, float scale) {
            return std::max(1, static_cast<int>(std::lround(points * scale)));
        }
    } // namespace

    CCViewportFrameNode* CCViewportFrameNode::create(float width, float height) {
        auto* node = new CCViewportFrameNode();
        if (node != nullptr && node->initWithSize(width, height)) {
            node->autorelease();
            return node;
        }
        delete node;
        return nullptr;
    }

    CCViewportFrameNode::CCViewportFrameNode() = default;

    CCViewportFrameNode::~CCViewportFrameNode() {
        destroyFramebuffer();
    }

    bool CCViewportFrameNode::initWithSize(float width, float height) {
        if (!CCNode::init()) {
            return false;
        }
        setAnchorPoint(ccp(0.0f, 0.0f));
        setContentSize(CCSizeMake(width, height));
        return true;
    }

    void CCViewportFrameNode::setCamera3D(Camera3D const& camera) {
        m_camera = camera;
    }

    Camera3D const& CCViewportFrameNode::getCamera3D() const {
        return m_camera;
    }

    int CCViewportFrameNode::addInstance(
        std::uint64_t meshId, Transform const& transform, glm::vec3 color
    ) {
        int const id = m_nextInstanceId++;
        m_instances.emplace(id, ViewportInstance{meshId, transform, color});
        return id;
    }

    bool CCViewportFrameNode::setInstanceTransform(int instanceId, Transform const& transform) {
        auto it = m_instances.find(instanceId);
        if (it == m_instances.end()) {
            return false;
        }
        it->second.transform = transform;
        return true;
    }

    bool CCViewportFrameNode::removeInstance(int instanceId) {
        return m_instances.erase(instanceId) > 0;
    }

    void CCViewportFrameNode::clearInstances() {
        m_instances.clear();
    }

    std::map<int, ViewportInstance> const& CCViewportFrameNode::instances() const {
        return m_instances;
    }

    unsigned int CCViewportFrameNode::framebuffer() const {
        return m_fbo;
    }

    unsigned int CCViewportFrameNode::colorTexture() const {
        return m_colorTexture;
    }

    int CCViewportFrameNode::framebufferPixelWidth() const {
        return m_fboPixelWidth;
    }

    int CCViewportFrameNode::framebufferPixelHeight() const {
        return m_fboPixelHeight;
    }

    void CCViewportFrameNode::setContentSize(CCSize const& size) {
        CCNode::setContentSize(size);
        invalidateFramebuffer();
    }

    void CCViewportFrameNode::draw() {
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
        renderer.renderToFramebuffer(m_fbo, m_fboPixelWidth, m_fboPixelHeight, m_camera, m_instances);
        renderer.drawCompositeQuad(m_colorTexture, size.width, size.height);
    }

    bool CCViewportFrameNode::hasGlContext() const {
        auto* director = CCDirector::sharedDirector();
        return director != nullptr && director->getOpenGLView() != nullptr;
    }

    void CCViewportFrameNode::invalidateFramebuffer() {
        destroyFramebuffer();
    }

    void CCViewportFrameNode::ensureFramebuffer() {
        if (!hasGlContext()) {
            return;
        }

        float const scale = CC_CONTENT_SCALE_FACTOR();
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
                "CCViewportFrameNode: framebuffer incomplete (status={:#x})",
                static_cast<unsigned>(status)
            );
            destroyFramebuffer();
            return;
        }

        m_fboPixelWidth = width;
        m_fboPixelHeight = height;
    }

    void CCViewportFrameNode::destroyFramebuffer() {
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
