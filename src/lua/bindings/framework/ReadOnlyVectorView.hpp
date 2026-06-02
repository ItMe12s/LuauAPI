#pragma once

#include "Usertype.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <lualib.h>

#include <cstddef>
#include <new>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace luax {
    namespace detail {
        template <class T>
        struct ReadOnlyVectorViewBlock {
            gd::vector<T*> const* vector = nullptr;
            geode::WeakRef<cocos2d::CCObject> owner;
        };

        template <class T>
        char const* readOnlyVectorViewMetatable() {
            static auto const name = std::string("luax:ReadOnlyVectorView<") + typeid(T).name() + ">";
            return name.c_str();
        }

        template <class T>
        ReadOnlyVectorViewBlock<T>* checkReadOnlyVectorView(lua_State* L, int idx) {
            auto* block = static_cast<ReadOnlyVectorViewBlock<T>*>(
                luaL_checkudata(L, idx, readOnlyVectorViewMetatable<T>())
            );
            if (!block || !block->vector || !block->owner.lock().data()) {
                luaL_error(L, "read-only vector view is no longer live");
            }
            return block;
        }

        template <class T>
        int readOnlyVectorViewIndex(lua_State* L) {
            auto* block = checkReadOnlyVectorView<T>(L, 1);
            auto index = luaL_checkinteger(L, 2);
            if (index < 1 || static_cast<std::size_t>(index) > block->vector->size()) {
                lua_pushnil(L);
                return 1;
            }
            Usertype<T>::pushBorrowed(L, (*block->vector)[static_cast<std::size_t>(index - 1)]);
            return 1;
        }

        template <class T>
        int readOnlyVectorViewLen(lua_State* L) {
            auto* block = checkReadOnlyVectorView<T>(L, 1);
            lua_pushinteger(L, static_cast<lua_Integer>(block->vector->size()));
            return 1;
        }

        template <class T>
        int readOnlyVectorViewNewIndex(lua_State* L) {
            luaL_error(L, "read-only vector view cannot be modified");
            return 0;
        }

        template <class T>
        int readOnlyVectorViewGc(lua_State* L) {
            auto* block = static_cast<ReadOnlyVectorViewBlock<T>*>(
                luaL_checkudata(L, 1, readOnlyVectorViewMetatable<T>())
            );
            if (block) block->~ReadOnlyVectorViewBlock<T>();
            return 0;
        }

        template <class T>
        void ensureReadOnlyVectorViewMetatable(lua_State* L) {
            if (luaL_newmetatable(L, readOnlyVectorViewMetatable<T>())) {
                lua_pushcfunction(L, &readOnlyVectorViewIndex<T>, "__index");
                lua_setfield(L, -2, "__index");
                lua_pushcfunction(L, &readOnlyVectorViewLen<T>, "__len");
                lua_setfield(L, -2, "__len");
                lua_pushcfunction(L, &readOnlyVectorViewNewIndex<T>, "__newindex");
                lua_setfield(L, -2, "__newindex");
                lua_pushcfunction(L, &readOnlyVectorViewGc<T>, "__gc");
                lua_setfield(L, -2, "__gc");
                lua_pushstring(L, "locked");
                lua_setfield(L, -2, "__metatable");
                lua_pushstring(L, "ReadOnlyVectorView");
                lua_setfield(L, -2, "__type");
            }
            lua_pop(L, 1);
        }
    }

    template <class T>
    void pushReadOnlyVectorView(lua_State* L, gd::vector<T*> const& vector, cocos2d::CCObject* owner) {
        static_assert(std::is_base_of_v<cocos2d::CCObject, T>, "vector view elements must be CCObject");
        if (!owner) {
            lua_pushnil(L);
            return;
        }
        detail::ensureReadOnlyVectorViewMetatable<T>(L);
        auto* storage = lua_newuserdata(L, sizeof(detail::ReadOnlyVectorViewBlock<T>));
        auto* block = new (storage) detail::ReadOnlyVectorViewBlock<T>();
        block->vector = &vector;
        block->owner = geode::WeakRef<cocos2d::CCObject>(owner);
        luaL_getmetatable(L, detail::readOnlyVectorViewMetatable<T>());
        lua_setmetatable(L, -2);
    }
}
