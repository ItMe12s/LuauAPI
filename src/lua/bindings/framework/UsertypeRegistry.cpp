#include "Usertype.hpp"

#ifndef LUAUAPI_HOST_TESTS
#include <Geode/Geode.hpp>
#endif

#include <new>

namespace luax::detail {
    UsertypeRegistry& UsertypeRegistry::get() {
        static auto* instance = new UsertypeRegistry;
        return *instance;
    }

    std::uint32_t UsertypeRegistry::tagFor(std::type_index idx) {
        auto it = m_byType.find(idx);
        if (it != m_byType.end()) return it->second.tag;
        TypeInfo info;
        info.tag = m_next++;
        if (info.tag >= LUA_UTAG_LIMIT) {
#ifndef LUAUAPI_HOST_TESTS
            geode::log::error("UsertypeRegistry: tag {} exceeds LUA_UTAG_LIMIT ({})", info.tag, LUA_UTAG_LIMIT);
#endif
            --m_next;
            return 0;
        }
        auto inserted = m_byType.emplace(idx, std::move(info));
        m_byTag.insert_or_assign(inserted.first->second.tag, idx);
        return inserted.first->second.tag;
    }

    TypeInfo& UsertypeRegistry::infoFor(std::type_index idx) {
        auto it = m_byType.find(idx);
        if (it != m_byType.end()) return it->second;
        TypeInfo info;
        info.tag = m_next++;
        if (info.tag >= LUA_UTAG_LIMIT) {
#ifndef LUAUAPI_HOST_TESTS
            geode::log::error("UsertypeRegistry: tag {} exceeds LUA_UTAG_LIMIT ({})", info.tag, LUA_UTAG_LIMIT);
#endif
            --m_next;
            static TypeInfo invalid{};
            return invalid;
        }
        auto inserted = m_byType.emplace(idx, std::move(info));
        m_byTag.insert_or_assign(inserted.first->second.tag, idx);
        return inserted.first->second;
    }

    TypeInfo const* UsertypeRegistry::findByTag(std::uint32_t tag) const {
        auto tagIt = m_byTag.find(tag);
        if (tagIt == m_byTag.end()) return nullptr;
        auto typeIt = m_byType.find(tagIt->second);
        return typeIt == m_byType.end() ? nullptr : &typeIt->second;
    }
}
