#include "render3d/TextureAsset.hpp"

namespace luax::render3d {

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
