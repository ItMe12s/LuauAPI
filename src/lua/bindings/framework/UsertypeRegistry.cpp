#include "Usertype.hpp"

#include <Geode/Geode.hpp>

#include <fmt/format.h>
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
            geode::log::error("UsertypeRegistry: tag {} exceeds LUA_UTAG_LIMIT ({})", info.tag, LUA_UTAG_LIMIT);
            --m_next;
            return 0;
        }
        auto inserted = m_byType.emplace(idx, std::move(info));
        m_byTag.insert_or_assign(inserted.first->second.tag, idx);
        return inserted.first->second.tag;
    }

    geode::Result<TypeInfo*> UsertypeRegistry::ensureInfo(std::type_index idx) {
        if (auto it = m_byType.find(idx); it != m_byType.end()) {
            return geode::Ok(&it->second);
        }
        if (m_next >= LUA_UTAG_LIMIT) {
            return geode::Err(fmt::format(
                "UsertypeRegistry: userdata tag limit exceeded ({})",
                LUA_UTAG_LIMIT
            ));
        }
        TypeInfo info;
        info.tag = m_next++;
        auto inserted = m_byType.emplace(idx, std::move(info));
        m_byTag.insert_or_assign(inserted.first->second.tag, idx);
        return geode::Ok(&inserted.first->second);
    }

    TypeInfo const* UsertypeRegistry::findInfo(std::type_index idx) const {
        auto it = m_byType.find(idx);
        return it == m_byType.end() ? nullptr : &it->second;
    }

    TypeInfo const* UsertypeRegistry::findByTag(std::uint32_t tag) const {
        auto tagIt = m_byTag.find(tag);
        if (tagIt == m_byTag.end()) return nullptr;
        auto typeIt = m_byType.find(tagIt->second);
        return typeIt == m_byType.end() ? nullptr : &typeIt->second;
    }

#if defined(LUAUAPI_HOST_TESTS)
    void UsertypeRegistry::setNextTagForTests(std::uint32_t tag) {
        m_next = tag;
    }
#endif
}
