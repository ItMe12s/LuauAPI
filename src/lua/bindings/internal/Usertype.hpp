#pragma once

#include "Ref.hpp"

#include <cocos2d.h>
#include <lua.h>
#include <lualib.h>

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace luax {
    struct UserdataBlock {
        cocos2d::CCObject* ptr;
        // Bit 0 set means Lua holds a retain on the object and the dtor must release it.
        std::uint32_t flags;
    };

    namespace detail {
        struct TypeInfo {
            std::uint32_t tag = 0;
            std::string name;
            std::string mtName;
            std::vector<std::uint32_t> baseClosure;
        };

        class UsertypeRegistry {
        public:
            static UsertypeRegistry& get();

            std::uint32_t tagFor(std::type_index idx);
            TypeInfo& infoFor(std::type_index idx);
            TypeInfo const* findByTag(std::uint32_t tag) const;

        private:
            std::unordered_map<std::type_index, TypeInfo> m_byType;
            std::unordered_map<std::uint32_t, TypeInfo*> m_byTag;
            std::uint32_t m_next = 1;
        };

        void destructorDispatch(lua_State* L, void* ud);
        void getOrCreateMetatable(lua_State* L, TypeInfo& info);
        void chainMethodTable(lua_State* L, TypeInfo const& info, std::uint32_t baseTag);
        void appendMethod(lua_State* L, TypeInfo const& info, char const* name, lua_CFunction fn);
        cocos2d::CCObject* checkUserdata(lua_State* L, int idx, std::uint32_t targetTag, char const* targetName, char const* method);
        void pushUserdata(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info, bool owned);
    }

    template <class T>
    struct Usertype {
        static_assert(std::is_base_of_v<cocos2d::CCObject, T>, "Usertype<T> requires T : CCObject");

        static std::uint32_t tag() {
            return detail::UsertypeRegistry::get().tagFor(std::type_index(typeid(T)));
        }

        static char const* name() {
            return detail::UsertypeRegistry::get().infoFor(std::type_index(typeid(T))).name.c_str();
        }

        static void registerType(lua_State* L, char const* nm, std::initializer_list<std::uint32_t> baseTags = {}) {
            auto& reg = detail::UsertypeRegistry::get();
            auto& info = reg.infoFor(std::type_index(typeid(T)));
            // Force a tag allocation in case infoFor returned a freshly seeded record.
            (void)reg.tagFor(std::type_index(typeid(T)));
            info.name = nm;
            info.mtName = std::string("luax:") + nm;

            info.baseClosure.clear();
            info.baseClosure.push_back(info.tag);
            for (std::uint32_t b : baseTags) {
                if (auto const* base = reg.findByTag(b)) {
                    for (std::uint32_t x : base->baseClosure) {
                        if (std::find(info.baseClosure.begin(), info.baseClosure.end(), x) == info.baseClosure.end()) {
                            info.baseClosure.push_back(x);
                        }
                    }
                } else if (std::find(info.baseClosure.begin(), info.baseClosure.end(), b) == info.baseClosure.end()) {
                    info.baseClosure.push_back(b);
                }
            }

            detail::getOrCreateMetatable(L, info);
            if (baseTags.size() > 0) {
                detail::chainMethodTable(L, info, *baseTags.begin());
            }
            lua_setuserdatadtor(L, static_cast<int>(info.tag), &detail::destructorDispatch);
        }

        static void method(lua_State* L, char const* methodName, lua_CFunction fn) {
            auto const& info = detail::UsertypeRegistry::get().infoFor(std::type_index(typeid(T)));
            detail::appendMethod(L, info, methodName, fn);
        }

        static T* check(lua_State* L, int idx, char const* method) {
            auto myTag = tag();
            auto* obj = detail::checkUserdata(L, idx, myTag, name(), method);
            return static_cast<T*>(obj);
        }

        static void pushOwned(lua_State* L, T* obj) {
            if (!obj) { lua_pushnil(L); return; }
            auto const& info = detail::UsertypeRegistry::get().infoFor(std::type_index(typeid(T)));
            retainLuaRef(static_cast<cocos2d::CCObject*>(obj), info.name.c_str());
            detail::pushUserdata(L, static_cast<cocos2d::CCObject*>(obj), info, true);
        }

        static void pushBorrowed(lua_State* L, T* obj) {
            if (!obj) { lua_pushnil(L); return; }
            auto const& info = detail::UsertypeRegistry::get().infoFor(std::type_index(typeid(T)));
            detail::pushUserdata(L, static_cast<cocos2d::CCObject*>(obj), info, false);
        }
    };
}
