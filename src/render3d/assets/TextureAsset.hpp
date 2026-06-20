#pragma once

#include "render3d/assets/AssetRegistry.hpp"
#include "render3d/assets/MeshAsset.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

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

        struct ViewportBinding;
        std::shared_ptr<ViewportBinding> m_viewportBinding;
    };

    using TextureRegistry = AssetRegistry<TextureAsset>;

} // namespace luax::render3d
