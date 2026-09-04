#pragma once

#include "render3d/assets/ImageDecode.hpp"

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>

namespace luax::render3d {

    class CCViewportFrame;

    struct TextureAsset {
        ImageData cpu;

        CCViewportFrame* viewportSource() const;
        unsigned int viewportColorTexture() const;

    private:
        friend class CCViewportFrame;

        void setViewportSourceNode(cocos2d::CCNode* node);

        geode::WeakRef<cocos2d::CCNode> m_viewportSource;
    };

} // namespace luax::render3d
