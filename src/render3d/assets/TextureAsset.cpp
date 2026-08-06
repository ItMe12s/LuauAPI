#include "render3d/assets/TextureAsset.hpp"

#if !defined(LUAUAPI_HOST_TESTS)
    #include "render3d/viewport/CCViewportFrame.hpp"

    #include <Geode/Geode.hpp>
    #include <cocos2d.h>
#endif

namespace luax::render3d {

#if !defined(LUAUAPI_HOST_TESTS)
    void TextureAsset::setViewportSourceNode(cocos2d::CCNode* node) {
        m_viewportSource = geode::WeakRef<cocos2d::CCNode>(node);
    }

    CCViewportFrame* TextureAsset::viewportSource() const {
        auto node = m_viewportSource.lock();
        if (!node) {
            return nullptr;
        }
        return geode::cast::typeinfo_cast<CCViewportFrame*>(node.data());
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
