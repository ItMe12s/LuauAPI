#pragma once

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <deque>
#include <type_traits>

namespace luax::detail {
    template <class T>
    inline void parkWeakRefForPoolSafety(geode::WeakRef<T>&& ref) {
        static_assert(
            std::is_base_of_v<cocos2d::CCObject, T>,
            "parkWeakRefForPoolSafety requires CCObject-derived T"
        );
        static auto* const parked = [] {
            return new std::deque<geode::WeakRef<T>>();
        }();
        parked->push_back(std::move(ref));
    }
} // namespace luax::detail
