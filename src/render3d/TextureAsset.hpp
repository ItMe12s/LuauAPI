#pragma once

#include "render3d/MeshAsset.hpp"

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

    private:
        friend class CCViewportFrame;

        void setViewportSourceNode(cocos2d::CCNode* node);

        struct ViewportBinding;
        std::shared_ptr<ViewportBinding> m_viewportBinding;
    };

    class TextureRegistry {
    public:
        static TextureRegistry& instance();

        std::uint64_t registerTexture(std::shared_ptr<TextureAsset> texture);
        void release(std::uint64_t id);
        std::shared_ptr<TextureAsset> get(std::uint64_t id) const;

    private:
        TextureRegistry() = default;

        std::uint64_t m_nextId = 1;
        std::unordered_map<std::uint64_t, std::shared_ptr<TextureAsset>> m_textures;
    };

} // namespace luax::render3d
