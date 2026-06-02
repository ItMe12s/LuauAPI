#pragma once

#include "Usertype.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <lualib.h>

#include <cstddef>
#include <memory>
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
            std::unique_ptr<gd::vector<T*>> owned;
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
            if (!block || !block->vector) {
                luaL_error(L, "read-only vector view is no longer live");
            }
            if (block->owned) {
                return block;
            }
            if (!block->owner.lock().data()) {
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

        template <class T>
        bool tryCopyReadOnlyVectorView(lua_State* L, int idx, gd::vector<T*>& out) {
            if (!lua_isuserdata(L, idx)) {
                return false;
            }
            auto* block = static_cast<ReadOnlyVectorViewBlock<T>*>(nullptr);
            if (lua_getmetatable(L, idx)) {
                luaL_getmetatable(L, readOnlyVectorViewMetatable<T>());
                if (lua_rawequal(L, -1, -2)) {
                    block = static_cast<ReadOnlyVectorViewBlock<T>*>(lua_touserdata(L, idx));
                }
                lua_pop(L, 2);
            }
            if (!block || !block->vector) {
                return false;
            }
            if (!block->owned && !block->owner.lock().data()) {
                return false;
            }
            out = *block->vector;
            return true;
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
        block->owned = std::make_unique<gd::vector<T*>>(vector);
        block->vector = block->owned.get();
        block->owner = geode::WeakRef<cocos2d::CCObject>(owner);
        luaL_getmetatable(L, detail::readOnlyVectorViewMetatable<T>());
        lua_setmetatable(L, -2);
    }

    template <class T>
    void pushOwnedReadOnlyVectorView(lua_State* L, gd::vector<T*> const& vector) {
        static_assert(std::is_base_of_v<cocos2d::CCObject, T>, "vector view elements must be CCObject");
        detail::ensureReadOnlyVectorViewMetatable<T>(L);
        auto* storage = lua_newuserdata(L, sizeof(detail::ReadOnlyVectorViewBlock<T>));
        auto* block = new (storage) detail::ReadOnlyVectorViewBlock<T>();
        block->owned = std::make_unique<gd::vector<T*>>(vector);
        block->vector = block->owned.get();
        luaL_getmetatable(L, detail::readOnlyVectorViewMetatable<T>());
        lua_setmetatable(L, -2);
    }

    template <class T>
    gd::vector<T*> checkObjectVectorView(lua_State* L, int idx, char const* label) {
        static_assert(std::is_base_of_v<cocos2d::CCObject, T>, "vector view elements must be CCObject");
        idx = lua_absindex(L, idx);
        gd::vector<T*> out;
        if (detail::tryCopyReadOnlyVectorView<T>(L, idx, out)) {
            return out;
        }
        luaL_checktype(L, idx, LUA_TTABLE);
        auto len = static_cast<lua_Integer>(lua_objlen(L, idx));
        if (len < 0) {
            luaL_error(L, "%s: invalid vector length", label);
        }
        out.reserve(static_cast<std::size_t>(len));
        for (lua_Integer i = 1; i <= len; ++i) {
            lua_rawgeti(L, idx, i);
            if (lua_isnil(L, -1)) {
                out.push_back(nullptr);
            } else {
                out.push_back(Usertype<T>::check(L, -1, label));
            }
            lua_pop(L, 1);
        }
        return out;
    }
}
