#include "framework/usertype/Usertype.hpp"

#include <fmt/format.h>
#include <new>
#include <string>
#include <string_view>

namespace luax::detail {
    UsertypeRegistry& UsertypeRegistry::get() {
        static auto* instance = new UsertypeRegistry;
        return *instance;
    }

    std::uint32_t UsertypeRegistry::tagFor(std::type_index idx) {
        auto result = ensureInfo(idx);
        if (result.isErr()) {
            return 0;
        }
        auto const* info = result.unwrap();
        if (!info || !isValidUserdataTag(info->tag)) {
            return 0;
        }
        return info->tag;
    }

    geode::Result<TypeInfo*> UsertypeRegistry::ensureInfo(std::type_index idx) {
        if (auto it = m_byType.find(idx); it != m_byType.end()) {
            return geode::Ok(&it->second);
        }
        if (m_next >= LUA_UTAG_LIMIT) {
            return geode::Err(
                fmt::format("UsertypeRegistry: userdata tag limit exceeded ({})", LUA_UTAG_LIMIT)
            );
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

    void UsertypeRegistry::indexName(std::type_index idx, std::string_view name) {
        if (name.empty()) {
            return;
        }
        for (auto it = m_byName.begin(); it != m_byName.end();) {
            if (it->second == idx) {
                it = m_byName.erase(it);
            }
            else {
                ++it;
            }
        }
        m_byName.insert_or_assign(std::string(name), idx);
    }

    TypeInfo const* UsertypeRegistry::findByName(std::string_view name) const {
        if (name.empty()) {
            return nullptr;
        }
        auto it = m_byName.find(std::string(name));
        if (it != m_byName.end()) {
            return findInfo(it->second);
        }
        auto const sep = name.rfind("::");
        if (sep == std::string_view::npos) {
            return nullptr;
        }
        auto shortName = name.substr(sep + 2);
        auto shortIt = m_byName.find(std::string(shortName));
        return shortIt == m_byName.end() ? nullptr : findInfo(shortIt->second);
    }

#if defined(LUAUAPI_HOST_TESTS)
    void UsertypeRegistry::setNextTagForTests(std::uint32_t tag) {
        m_next = tag;
    }

    void UsertypeRegistry::resetForTests() {
        m_byType.clear();
        m_byTag.clear();
        m_byName.clear();
        m_next = kFirstDynamicUsertypeTag;
    }
#endif
} // namespace luax::detail
