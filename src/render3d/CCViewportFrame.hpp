#pragma once

#include "render3d/Material.hpp"
#include "render3d/Transform3D.hpp"

#include <cocos2d.h>
#include <cstdint>
#include <glm/vec3.hpp>
#include <map>
#include <memory>

namespace luax::render3d {

    class MeshAsset;

    struct Camera3D {
        Transform transform{};
        float fovYDegrees = 70.0f;
        float zNear = 0.1f;
        float zFar = 100.0f;
    };

    struct ViewportInstance {
        std::uint64_t meshId = 0;
        std::shared_ptr<MeshAsset> mesh{};
        Transform transform{};
        glm::vec3 color{1.0f, 1.0f, 1.0f};
        std::shared_ptr<Material> materialOverride{};
    };

    class CCViewportFrame final : public cocos2d::CCNode {
    public:
        static CCViewportFrame* create(float width, float height);

        void setCamera3D(Camera3D const& camera);
        Camera3D const& getCamera3D() const;

        int addInstance(
            std::uint64_t meshId, std::shared_ptr<MeshAsset> mesh, Transform const& transform,
            glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f)
        );
        bool setInstanceTransform(int instanceId, Transform const& transform);
        bool setInstanceMaterial(int instanceId, std::shared_ptr<Material> material);
        bool setInstanceColor(int instanceId, glm::vec3 color);
        bool removeInstance(int instanceId);
        void clearInstances();

        std::map<int, ViewportInstance> const& instances() const;

        unsigned int framebuffer() const;
        unsigned int colorTexture() const;
        int framebufferPixelWidth() const;
        int framebufferPixelHeight() const;

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
        std::map<int, ViewportInstance> m_instances{};
        int m_nextInstanceId = 1;

        unsigned int m_fbo = 0;
        unsigned int m_colorTexture = 0;
        unsigned int m_depthRenderbuffer = 0;
        int m_fboPixelWidth = 0;
        int m_fboPixelHeight = 0;
    };

} // namespace luax::render3d
