#include "render3d/assets/TextureAsset.hpp"

#if !defined(LUAUAPI_HOST_TESTS)
    #include "render3d/viewport/CCViewportFrame.hpp"

    #include <Geode/utils/cocos.hpp>
    #include <cocos2d.h>
#endif

namespace luax::render3d {

#if !defined(LUAUAPI_HOST_TESTS)

    struct TextureAsset::ViewportBinding {
        geode::WeakRef<cocos2d::CCNode> node;
    };

    void TextureAsset::setViewportSourceNode(cocos2d::CCNode* node) {
        auto binding = std::make_shared<ViewportBinding>();
        binding->node = geode::WeakRef<cocos2d::CCNode>(node);
        m_viewportBinding = std::move(binding);
    }

    CCViewportFrame* TextureAsset::viewportSource() const {
        if (!m_viewportBinding) {
            return nullptr;
        }
        auto node = m_viewportBinding->node.lock();
        if (!node) {
            return nullptr;
        }
        return static_cast<CCViewportFrame*>(node.data());
    }

    unsigned int TextureAsset::viewportColorTexture() const {
        auto* viewport = viewportSource();
        if (viewport == nullptr) {
            return 0;
        }
        return viewport->colorTexture();
    }

#else

    void TextureAsset::setViewportSourceNode(cocos2d::CCNode* node) {
        (void)node;
    }

    CCViewportFrame* TextureAsset::viewportSource() const {
        return nullptr;
    }

    unsigned int TextureAsset::viewportColorTexture() const {
        return 0;
    }

#endif

} // namespace luax::render3d
