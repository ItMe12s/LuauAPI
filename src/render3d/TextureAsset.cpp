#include "render3d/TextureAsset.hpp"

#if !defined(LUAUAPI_HOST_TESTS)
    #include "render3d/CCViewportFrame.hpp"

    #include <Geode/utils/cocos.hpp>
#endif
#include <cocos2d.h>

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

#else

    void TextureAsset::setViewportSourceNode(cocos2d::CCNode* node) {
        (void)node;
    }

    CCViewportFrame* TextureAsset::viewportSource() const {
        return nullptr;
    }

#endif

    TextureRegistry& TextureRegistry::instance() {
        static TextureRegistry registry;
        return registry;
    }

    std::uint64_t TextureRegistry::registerTexture(std::shared_ptr<TextureAsset> texture) {
        auto const id = m_nextId++;
        m_textures.emplace(id, std::move(texture));
        return id;
    }

    void TextureRegistry::release(std::uint64_t id) {
        m_textures.erase(id);
    }

    std::shared_ptr<TextureAsset> TextureRegistry::get(std::uint64_t id) const {
        auto const it = m_textures.find(id);
        if (it == m_textures.end()) {
            return nullptr;
        }
        return it->second;
    }

} // namespace luax::render3d
