#pragma once

#include "ReadOnlySequenceView.hpp"
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
#include <utility>

namespace luax {
    namespace detail {
        template <class T>
        struct CcCArrayBackingPolicy {
            struct Block {
                cocos2d::ccCArray const* array = nullptr;
                geode::WeakRef<cocos2d::CCObject> owner;
            };

            using BorrowedArgs = std::pair<cocos2d::ccCArray*, cocos2d::CCObject*>;

            static constexpr bool kSupportsCopy = false;
            static constexpr bool kSupportsBorrowed = true;
            static constexpr bool kSupportsOwned = false;
            static constexpr char const* kTypeName = "ReadOnlyCCArrayView";
            static constexpr char const* kLiveError = "read-only ccCArray view is no longer live";
            static constexpr char const* kNewIndexError =
                "read-only ccCArray view cannot be modified";

            static bool storageLive(Block const& block) {
                return block.array != nullptr;
            }

            static bool ownerLive(Block const& block) {
                return block.owner.lock().data() != nullptr;
            }

            static std::size_t size(Block const& block) {
                return static_cast<std::size_t>(block.array->num);
            }

            static T* at(Block const& block, std::size_t index) {
                return static_cast<T*>(block.array->arr[index]);
            }

            static bool canPushBorrowed(BorrowedArgs const& args) {
                return args.first != nullptr && args.second != nullptr;
            }

            static void initBorrowed(Block& block, BorrowedArgs const& args) {
                block.array = args.first;
                block.owner = geode::WeakRef<cocos2d::CCObject>(args.second);
            }
        };

        struct CCArrayObjectPolicy {
            static constexpr char const* kPrefix = "luax:ReadOnlyCCArrayView<";

            template <class U>
            static void pushElement(lua_State* L, U* value) {
                if (value == nullptr) {
                    lua_pushnil(L);
                }
                else {
                    Usertype<U>::pushBorrowed(L, value);
                }
            }
        };
    } // namespace detail

    template <class T>
    void pushReadOnlyCCArrayView(lua_State* L, cocos2d::ccCArray* array, cocos2d::CCObject* owner) {
        static_assert(
            std::is_base_of_v<cocos2d::CCObject, T>, "ccCArray view elements must be CCObject"
        );
        detail::pushBorrowedSequenceView<T, detail::CcCArrayBackingPolicy<T>, detail::CCArrayObjectPolicy>(
            L, std::pair<cocos2d::ccCArray*, cocos2d::CCObject*>(array, owner)
        );
    }
} // namespace luax
