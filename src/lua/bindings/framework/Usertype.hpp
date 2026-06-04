#pragma once

#include "Ref.hpp"

#include <Geode/Result.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <fmt/format.h>
#include <lua.h>
#include <lualib.h>

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace luax {
    struct UserdataBlock {
        cocos2d::CCObject* ptr = nullptr;
        geode::WeakRef<cocos2d::CCObject> weak;
        std::uint32_t flags = 0;
    };

    namespace detail {
        constexpr bool isValidUserdataTag(std::uint32_t tag) noexcept {
            return tag != 0 && tag < LUA_UTAG_LIMIT;
        }

        struct TypeInfo {
            std::uint32_t tag = 0;
            std::string name;
            std::string mtName;
            std::vector<std::uint32_t> baseClosure;
            bool isNode = false;
        };

        struct UserdataCandidate {
            cocos2d::CCObject* obj = nullptr;
            TypeInfo const* info = nullptr;
        };

        class UsertypeRegistry {
        public:
            static UsertypeRegistry& get();

            std::uint32_t tagFor(std::type_index idx);
            geode::Result<TypeInfo*> ensureInfo(std::type_index idx);
            TypeInfo const* findInfo(std::type_index idx) const;
            TypeInfo const* findByTag(std::uint32_t tag) const;
#if defined(LUAUAPI_HOST_TESTS)
            void setNextTagForTests(std::uint32_t tag);
            void resetForTests();
#endif

        private:
            std::unordered_map<std::type_index, TypeInfo> m_byType;
            std::unordered_map<std::uint32_t, std::type_index> m_byTag;
            std::uint32_t m_next = 1;
        };

        cocos2d::CCObject* liveObject(UserdataBlock* block);
        void destructorDispatch(lua_State* L, void* ud);
        void getOrCreateMetatable(lua_State* L, TypeInfo& info);
        geode::Result<void> ensureUserdataMetatable(lua_State* L, TypeInfo const& info);
        void chainMethodTable(lua_State* L, TypeInfo const& info, std::uint32_t baseTag);
        void appendMethod(lua_State* L, TypeInfo const& info, char const* name, lua_CFunction fn);
        void appendField(lua_State* L, TypeInfo const& info, char const* name, lua_CFunction getter, lua_CFunction setter);
        void appendReadOnlyField(lua_State* L, TypeInfo const& info, char const* name, lua_CFunction getter);
        UserdataCandidate checkCandidate(lua_State* L, int idx, char const* targetName, char const* method);
        UserdataCandidate tryCandidate(lua_State* L, int idx);
        cocos2d::CCNode* tryNodeCandidate(lua_State* L, int idx);
        bool hasBase(TypeInfo const& info, std::uint32_t targetTag);
        void pushUserdataOwned(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info);
        void pushUserdataBorrowed(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info);
    }

    template <class T>
    struct Usertype {
        static_assert(std::is_base_of_v<cocos2d::CCObject, T>, "Usertype<T> requires T : CCObject");

        static std::uint32_t tag() {
            if (auto const* info = detail::UsertypeRegistry::get().findInfo(std::type_index(typeid(T)))) {
                return detail::isValidUserdataTag(info->tag) ? info->tag : 0;
            }
            return 0;
        }

        static char const* name() {
            if (auto const* info = detail::UsertypeRegistry::get().findInfo(std::type_index(typeid(T)))) {
                return info->name.c_str();
            }
            return "";
        }

        static geode::Result<void> registerType(lua_State* L, char const* nm, std::initializer_list<std::uint32_t> baseTags = {}) {
            auto& reg = detail::UsertypeRegistry::get();
            auto infoResult = reg.ensureInfo(std::type_index(typeid(T)));
            if (infoResult.isErr()) {
                return geode::Err(infoResult.unwrapErr());
            }
            auto& info = *infoResult.unwrap();
            if (!detail::isValidUserdataTag(info.tag)) {
                return geode::Err(fmt::format("{} invalid userdata tag {}", nm, info.tag));
            }
            info.name = nm;
            info.mtName = std::string("luax:") + nm;
            info.isNode = std::is_base_of_v<cocos2d::CCNode, T>;

            info.baseClosure.clear();
            info.baseClosure.push_back(info.tag);
            for (std::uint32_t b : baseTags) {
                if (!detail::isValidUserdataTag(b)) {
                    return geode::Err(fmt::format("{} invalid base userdata tag {}", nm, b));
                }
                if (auto const* base = reg.findByTag(b)) {
                    for (std::uint32_t x : base->baseClosure) {
                        if (std::find(info.baseClosure.begin(), info.baseClosure.end(), x) == info.baseClosure.end()) {
                            info.baseClosure.push_back(x);
                        }
                    }
                } else {
                    return geode::Err(fmt::format("{} unknown base userdata tag {}", nm, b));
                }
            }

            detail::getOrCreateMetatable(L, info);
            if (baseTags.size() > 0) {
                detail::chainMethodTable(L, info, *baseTags.begin());
            }
            auto mtResult = detail::ensureUserdataMetatable(L, info);
            if (mtResult.isErr()) {
                return geode::Err(mtResult.unwrapErr());
            }
            lua_setuserdatadtor(L, static_cast<int>(info.tag), &detail::destructorDispatch);
            return geode::Ok();
        }

        static void method(lua_State* L, char const* methodName, lua_CFunction fn) {
            auto const* info = detail::UsertypeRegistry::get().findInfo(std::type_index(typeid(T)));
            if (!info) return;
            detail::appendMethod(L, *info, methodName, fn);
        }

        static void field(lua_State* L, char const* fieldName, lua_CFunction getter, lua_CFunction setter) {
            auto const* info = detail::UsertypeRegistry::get().findInfo(std::type_index(typeid(T)));
            if (!info) return;
            detail::appendField(L, *info, fieldName, getter, setter);
        }

        static void readonlyField(lua_State* L, char const* fieldName, lua_CFunction getter) {
            auto const* info = detail::UsertypeRegistry::get().findInfo(std::type_index(typeid(T)));
            if (!info) return;
            detail::appendReadOnlyField(L, *info, fieldName, getter);
        }

        static T* check(lua_State* L, int idx, char const* method) {
            auto candidate = detail::checkCandidate(L, idx, name(), method);
            if (auto* typed = geode::cast::typeinfo_cast<T*>(candidate.obj)) {
                return typed;
            }
            if (detail::hasBase(*candidate.info, tag())) {
                return static_cast<T*>(candidate.obj);
            }
            luaL_error(L, "%s expected %s at arg %d", method, name(), idx);
        }

        static T* tryCheck(lua_State* L, int idx) {
            auto candidate = detail::tryCandidate(L, idx);
            if (!candidate.obj || !candidate.info) return nullptr;
            if (auto* typed = geode::cast::typeinfo_cast<T*>(candidate.obj)) {
                return typed;
            }
            return detail::hasBase(*candidate.info, tag()) ? static_cast<T*>(candidate.obj) : nullptr;
        }

        static void pushOwned(lua_State* L, T* obj) {
            if (!obj) { lua_pushnil(L); return; }
            auto const* info = detail::UsertypeRegistry::get().findInfo(std::type_index(typeid(T)));
            if (!info || !detail::isValidUserdataTag(info->tag)) { lua_pushnil(L); return; }
            detail::pushUserdataOwned(L, static_cast<cocos2d::CCObject*>(obj), *info);
            if (!retainLuaRef(static_cast<cocos2d::CCObject*>(obj), info->name.c_str())) {
                lua_pop(L, 1);
                lua_pushnil(L);
            }
        }

        static void pushBorrowed(lua_State* L, T* obj) {
            if (!obj) { lua_pushnil(L); return; }
            auto const* info = detail::UsertypeRegistry::get().findInfo(std::type_index(typeid(T)));
            if (!info || !detail::isValidUserdataTag(info->tag)) { lua_pushnil(L); return; }
            detail::pushUserdataBorrowed(L, static_cast<cocos2d::CCObject*>(obj), *info);
        }
    };
}
