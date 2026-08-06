#pragma once

#include "render3d/assets/AssetRegistry.hpp"
#include "render3d/assets/MeshAsset.hpp"

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <cstdint>

namespace cocos2d {
    class CCNode;
}

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

    using TextureRegistry = AssetRegistry<TextureAsset>;

} // namespace luax::render3d
