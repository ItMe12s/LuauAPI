#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace luax::render3d {

    template <typename Asset>
    class AssetRegistry {
    public:
        static AssetRegistry& instance() {
            static AssetRegistry registry;
            return registry;
        }

        std::uint64_t registerAsset(std::shared_ptr<Asset> asset) {
            auto const id = m_nextId++;
            m_assets.emplace(id, std::move(asset));
            return id;
        }

        void release(std::uint64_t id) {
            m_assets.erase(id);
        }

        std::shared_ptr<Asset> get(std::uint64_t id) const {
            auto const it = m_assets.find(id);
            if (it == m_assets.end()) {
                return nullptr;
            }
            return it->second;
        }

    private:
        AssetRegistry() = default;

        std::uint64_t m_nextId = 1;
        std::unordered_map<std::uint64_t, std::shared_ptr<Asset>> m_assets;
    };

} // namespace luax::render3d
