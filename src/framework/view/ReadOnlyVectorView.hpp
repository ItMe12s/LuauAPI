#pragma once

#include "framework/usertype/OpaqueHandle.hpp"
#include "framework/usertype/Usertype.hpp"
#include "framework/view/ReadOnlySequenceView.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <cstddef>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace luax {
    namespace detail {
        template <class U>
        void pushCCObjectElement(lua_State* L, U* value) {
            if (value == nullptr) {
                lua_pushnil(L);
            }
            else if constexpr (std::is_same_v<U, cocos2d::CCObject>) {
                Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, value);
            }
            else {
                Usertype<U>::pushBorrowed(L, value);
            }
        }

        template <class T>
        struct GdVectorBackingPolicy {
            struct Block {
                gd::vector<T*> const* vector = nullptr;
                geode::WeakRef<cocos2d::CCObject> owner;
                std::unique_ptr<gd::vector<T*>> owned;
            };

            using CopyOut = gd::vector<T*>;
            using BorrowedArgs = std::pair<gd::vector<T*> const&, cocos2d::CCObject*>;
            using OwnedArgs = gd::vector<T*>;

            static constexpr bool kSupportsCopy = true;
            static constexpr bool kSupportsBorrowed = true;
            static constexpr bool kSupportsOwned = true;
            static constexpr char const* kTypeName = "ReadOnlyVectorView";
            static constexpr char const* kLiveError = "read-only vector view is no longer live";
            static constexpr char const* kNewIndexError =
                "read-only vector view cannot be modified";

            static bool storageLive(Block const& block) {
                return block.vector != nullptr;
            }

            static bool ownerLive(Block const& block) {
                return static_cast<bool>(block.owned) || block.owner.lock().data() != nullptr;
            }

            static std::size_t size(Block const& block) {
                return block.vector->size();
            }

            static T* at(Block const& block, std::size_t index) {
                return (*block.vector)[index];
            }

            static void copyOut(Block const& block, CopyOut& out) {
                out = *block.vector;
            }

            static bool canPushBorrowed(BorrowedArgs const& args) {
                return args.second != nullptr;
            }

            static void initBorrowed(Block& block, BorrowedArgs const& args) {
                block.vector = &args.first;
                block.owner = geode::WeakRef<cocos2d::CCObject>(args.second);
            }

            static void initOwned(Block& block, OwnedArgs const& args) {
                block.owned = std::make_unique<gd::vector<T*>>(args);
                block.vector = block.owned.get();
            }
        };

        struct VectorObjectPolicy {
            static constexpr char const* kPrefix = "luax:ReadOnlyVectorView<";

            template <class U>
            static void pushElement(lua_State* L, U* value) {
                pushCCObjectElement(L, value);
            }
        };

        struct VectorOpaquePolicy {
            static constexpr char const* kPrefix = "luax:ReadOnlyOpaqueVectorView<";

            template <class T>
            static void pushElement(lua_State* L, T* value) {
                if (value == nullptr) {
                    lua_pushnil(L);
                }
                else {
                    pushOpaqueHandle(L, value);
                }
            }
        };

        template <class T, class Policy>
        bool tryCopyReadOnlyVectorView(lua_State* L, int idx, gd::vector<T*>& out) {
            return tryCopyReadOnlySequenceView<T, GdVectorBackingPolicy<T>, Policy>(L, idx, out);
        }

        template <class T, class Policy>
        void pushBorrowedVectorView(lua_State* L, gd::vector<T*> const& vector, cocos2d::CCObject* owner) {
            pushBorrowedSequenceView<T, GdVectorBackingPolicy<T>, Policy>(
                L, std::pair<gd::vector<T*> const&, cocos2d::CCObject*>(vector, owner)
            );
        }

        template <class T, class Policy>
        void pushOwnedVectorView(lua_State* L, gd::vector<T*> const& vector) {
            pushOwnedSequenceView<T, GdVectorBackingPolicy<T>, Policy>(L, vector);
        }

        template <class T, class Policy, class CheckFn>
        gd::vector<T*> checkPointerVectorFromTable(
            lua_State* L, int idx, char const* label, CheckFn checkFn
        ) {
            idx = lua_absindex(L, idx);
            gd::vector<T*> out;
            if (tryCopyReadOnlyVectorView<T, Policy>(L, idx, out)) {
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
                }
                else {
                    out.push_back(checkFn(L, -1, label));
                }
                lua_pop(L, 1);
            }
            return out;
        }
    } // namespace detail

    // CCObject views: elements are Usertype-borrowed nodes, T must inherit CCObject.
    template <class T>
    void pushReadOnlyVectorView(lua_State* L, gd::vector<T*> const& vector, cocos2d::CCObject* owner) {
        static_assert(
            std::is_base_of_v<cocos2d::CCObject, T>, "vector view elements must be CCObject"
        );
        detail::pushBorrowedVectorView<T, detail::VectorObjectPolicy>(L, vector, owner);
    }

    template <class T>
    void pushReadOnlyVectorView(lua_State*, gd::vector<T*> const&&, cocos2d::CCObject*) = delete;

    template <class T>
    void pushOwnedReadOnlyVectorView(lua_State* L, gd::vector<T*> const& vector) {
        static_assert(
            std::is_base_of_v<cocos2d::CCObject, T>, "vector view elements must be CCObject"
        );
        detail::pushOwnedVectorView<T, detail::VectorObjectPolicy>(L, vector);
    }

    template <class T>
    gd::vector<T*> checkObjectVectorView(lua_State* L, int idx, char const* label) {
        static_assert(
            std::is_base_of_v<cocos2d::CCObject, T>, "vector view elements must be CCObject"
        );
        return detail::checkPointerVectorFromTable<T, detail::VectorObjectPolicy>(
            L, idx, label, [](lua_State* L, int i, char const* lbl) {
                return Usertype<T>::check(L, i, lbl);
            }
        );
    }

    // Opaque views: elements use tagged opaque handles instead of Usertype borrowing.
    // Codegen may route CCObject-derived pointer vectors here (e.g. std::vector in map values).
    // Borrowed opaque views still pin storage to a live CCObject owner (GdVectorBackingPolicy).
    template <class T>
    void pushReadOnlyOpaqueVectorView(
        lua_State* L, gd::vector<T*> const& vector, cocos2d::CCObject* owner
    ) {
        detail::pushBorrowedVectorView<T, detail::VectorOpaquePolicy>(L, vector, owner);
    }

    template <class T>
    void pushReadOnlyOpaqueVectorView(lua_State*, gd::vector<T*> const&&, cocos2d::CCObject*) = delete;

    template <class T>
    void pushOwnedReadOnlyOpaqueVectorView(lua_State* L, gd::vector<T*> const& vector) {
        detail::pushOwnedVectorView<T, detail::VectorOpaquePolicy>(L, vector);
    }

    template <class T>
    gd::vector<T*> checkOpaqueVectorView(lua_State* L, int idx, char const* label) {
        return detail::checkPointerVectorFromTable<T, detail::VectorOpaquePolicy>(
            L, idx, label, [](lua_State* L, int i, char const* lbl) {
                return checkOpaqueHandle<T>(L, i, lbl);
            }
        );
    }
} // namespace luax
