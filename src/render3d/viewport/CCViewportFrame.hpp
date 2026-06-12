#pragma once

#include "render3d/types/SceneTypes.hpp"

#include <cocos2d.h>
#include <map>
#include <memory>

namespace luax::render3d {

    class CCViewportFrame final : public cocos2d::CCNode {
    public:
        static CCViewportFrame* create(float width, float height);

        void setCamera3D(Camera3D const& camera);
        Camera3D const& getCamera3D() const;

        void setRenderSettings(RenderSettings const& settings);
        RenderSettings const& renderSettings() const;

        int addInstance(
            std::uint64_t meshId, std::shared_ptr<MeshAsset> mesh, Transform const& transform,
            glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f)
        );
        bool setInstanceTransform(int instanceId, Transform const& transform);
        bool setInstanceMaterial(int instanceId, std::shared_ptr<Material> material);
        bool setInstancePrimitiveMaterial(
            int instanceId, int primitiveIndex, std::shared_ptr<Material> material
        );
        bool setInstanceColor(int instanceId, glm::vec3 color);
        bool removeInstance(int instanceId);
        void clearInstances();

        std::map<int, ViewportInstance> const& instances() const;

        unsigned int framebuffer() const;
        unsigned int colorTexture() const;
        int framebufferPixelWidth() const;
        int framebufferPixelHeight() const;

        void setCompositeEnabled(bool enabled);
        bool compositeEnabled() const;

        int addDebugLine(glm::vec3 from, glm::vec3 to, glm::vec3 color);
        bool removeDebugLine(int lineId);
        void clearDebugLines();
        void setDebugBounds(bool enabled);
        bool debugBounds() const;
        std::map<int, DebugLine> const& debugLines() const;

        std::uint64_t ensureViewportTextureId();

        void draw() override;
        void setContentSize(cocos2d::CCSize const& size) override;

    protected:
        CCViewportFrame();
        ~CCViewportFrame() override;

        bool initWithSize(float width, float height);

    private:
        void ensureFramebuffer();
        void destroyFramebuffer();
        bool hasGlContext() const;
        void invalidateFramebuffer();

        Camera3D m_camera{};
        RenderSettings m_settings{};
        std::map<int, ViewportInstance> m_instances{};
        int m_nextInstanceId = 1;

        unsigned int m_fbo = 0;
        unsigned int m_colorTexture = 0;
        unsigned int m_depthRenderbuffer = 0;
        int m_fboPixelWidth = 0;
        int m_fboPixelHeight = 0;

        bool m_compositeEnabled = true;
        std::uint64_t m_viewportTextureId = 0;

        std::map<int, DebugLine> m_debugLines{};
        int m_nextDebugLineId = 1;
        bool m_debugBounds = false;
    };

} // namespace luax::render3d
