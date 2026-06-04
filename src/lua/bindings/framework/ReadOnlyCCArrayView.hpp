#pragma once

#include "Usertype.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <cstddef>
#include <lua.h>
#include <lualib.h>
#include <new>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace luax {
    namespace detail {
        template <class T>
        struct ReadOnlyCCArrayViewBlock {
            cocos2d::ccCArray const* array = nullptr;
            geode::WeakRef<cocos2d::CCObject> owner;
        };

        template <class T>
        char const* readOnlyCCArrayViewMetatable() {
            static auto const name =
                std::string("luax:ReadOnlyCCArrayView<") + typeid(T).name() + ">";
            return name.c_str();
        }

        template <class T>
        ReadOnlyCCArrayViewBlock<T>* checkReadOnlyCCArrayView(lua_State* L, int idx) {
            auto* block = static_cast<ReadOnlyCCArrayViewBlock<T>*>(
                luaL_checkudata(L, idx, readOnlyCCArrayViewMetatable<T>())
            );
            if (!block || !block->array) {
                luaL_error(L, "read-only ccCArray view is no longer live");
            }
            if (!block->owner.lock().data()) {
                luaL_error(L, "read-only ccCArray view is no longer live");
            }
            return block;
        }

        template <class T>
        int readOnlyCCArrayViewIndex(lua_State* L) {
            auto* block = checkReadOnlyCCArrayView<T>(L, 1);
            auto index = luaL_checkinteger(L, 2);
            if (index < 1 ||
                static_cast<std::size_t>(index) > static_cast<std::size_t>(block->array->num)) {
                lua_pushnil(L);
                return 1;
            }
            auto* value = static_cast<T*>(block->array->arr[static_cast<std::size_t>(index - 1)]);
            if (value == nullptr) {
                lua_pushnil(L);
            }
            else {
                Usertype<T>::pushBorrowed(L, value);
            }
            return 1;
        }

        template <class T>
        int readOnlyCCArrayViewLen(lua_State* L) {
            auto* block = checkReadOnlyCCArrayView<T>(L, 1);
            lua_pushinteger(L, static_cast<lua_Integer>(block->array->num));
            return 1;
        }

        template <class T>
        int readOnlyCCArrayViewNewIndex(lua_State* L) {
            luaL_error(L, "read-only ccCArray view cannot be modified");
            return 0;
        }

        template <class T>
        int readOnlyCCArrayViewGc(lua_State* L) {
            auto* block = static_cast<ReadOnlyCCArrayViewBlock<T>*>(
                luaL_checkudata(L, 1, readOnlyCCArrayViewMetatable<T>())
            );
            if (block) block->~ReadOnlyCCArrayViewBlock<T>();
            return 0;
        }

        template <class T>
        void ensureReadOnlyCCArrayViewMetatable(lua_State* L) {
            if (luaL_newmetatable(L, readOnlyCCArrayViewMetatable<T>())) {
                lua_pushcfunction(L, &readOnlyCCArrayViewIndex<T>, "__index");
                lua_setfield(L, -2, "__index");
                lua_pushcfunction(L, &readOnlyCCArrayViewLen<T>, "__len");
                lua_setfield(L, -2, "__len");
                lua_pushcfunction(L, &readOnlyCCArrayViewNewIndex<T>, "__newindex");
                lua_setfield(L, -2, "__newindex");
                lua_pushcfunction(L, &readOnlyCCArrayViewGc<T>, "__gc");
                lua_setfield(L, -2, "__gc");
                lua_pushstring(L, "locked");
                lua_setfield(L, -2, "__metatable");
                lua_pushstring(L, "ReadOnlyCCArrayView");
                lua_setfield(L, -2, "__type");
            }
            lua_pop(L, 1);
        }
    } // namespace detail

    template <class T>
    void pushReadOnlyCCArrayView(lua_State* L, cocos2d::ccCArray* array, cocos2d::CCObject* owner) {
        static_assert(
            std::is_base_of_v<cocos2d::CCObject, T>, "ccCArray view elements must be CCObject"
        );
        if (!array || !owner) {
            lua_pushnil(L);
            return;
        }
        detail::ensureReadOnlyCCArrayViewMetatable<T>(L);
        auto* storage = lua_newuserdata(L, sizeof(detail::ReadOnlyCCArrayViewBlock<T>));
        auto* block = new (storage) detail::ReadOnlyCCArrayViewBlock<T>();
        block->array = array;
        block->owner = geode::WeakRef<cocos2d::CCObject>(owner);
        luaL_getmetatable(L, detail::readOnlyCCArrayViewMetatable<T>());
        lua_setmetatable(L, -2);
    }
} // namespace luax
