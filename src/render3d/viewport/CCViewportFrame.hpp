#pragma once

#include "render3d/types/SceneTypes.hpp"

#include <cocos2d.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace luax::render3d {

    struct TextureAsset;

    void abandonLiveViewports();

    class CCViewportFrame final : public cocos2d::CCSprite {
    public:
        static CCViewportFrame* create(float width, float height);

        void setCamera3D(Camera3D const& camera);
        Camera3D const& getCamera3D() const;

        void setRenderSettings(RenderSettings const& settings);
        RenderSettings const& renderSettings() const;

        int addInstance(
            std::shared_ptr<MeshAsset> mesh, Transform const& transform,
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

        std::unordered_map<int, ViewportInstance> const& instances() const;

        unsigned int colorTexture() const;
        int framebufferPixelWidth() const;
        int framebufferPixelHeight() const;

        void setCompositeEnabled(bool enabled);

        int addDebugLine(glm::vec3 from, glm::vec3 to, glm::vec3 color);
        bool removeDebugLine(int lineId);
        void clearDebugLines();
        void setDebugBounds(bool enabled);

        std::shared_ptr<TextureAsset> viewportTextureAsset();

        void draw() override;
        void abandonGpuResources();

    protected:
        CCViewportFrame();
        ~CCViewportFrame() override;

        bool initWithSize(float width, float height);

    private:
        static constexpr int kMaxFramebufferDimension = 4096;

        bool gpuHandlesValid() const;

        void ensureFramebuffer();
        bool createFramebuffer(int pixelWidth, int pixelHeight);
        cocos2d::CCTexture2D* buildFramebufferTexture(int pixelWidth, int pixelHeight);
        void destroyFramebuffer();
        void refreshSpriteTexture(cocos2d::CCSize const& points);
        void detachSpriteTexture();
        void deleteColorTexture();

        Camera3D m_camera{};
        RenderSettings m_settings{};
        std::unordered_map<int, ViewportInstance> m_instances{};
        int m_nextInstanceId = 1;
        std::vector<int> m_instanceFreeList{};

        unsigned int m_fbo = 0;
        unsigned int m_colorTexture = 0;
        unsigned int m_depthRenderbuffer = 0;
        cocos2d::CCTexture2D* m_framebufferTexture = nullptr;
        int m_fboPixelWidth = 0;
        int m_fboPixelHeight = 0;
        unsigned m_glContextGeneration = 0;

        bool m_compositeEnabled = true;
        std::shared_ptr<TextureAsset> m_viewportTexture{};

        std::unordered_map<int, DebugLine> m_debugLines{};
        int m_nextDebugLineId = 1;
        std::vector<int> m_debugLineFreeList{};
        bool m_debugBounds = false;
    };

} // namespace luax::render3d
