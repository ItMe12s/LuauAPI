#pragma once

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <type_traits>
#include <vector>

namespace luax::detail {
    // I forgor :skull:
    template <class T>
    inline void leakWeakRefDuringShutdown(geode::WeakRef<T>&& ref) {
        static_assert(
            std::is_base_of_v<cocos2d::CCObject, T>,
            "leakWeakRefDuringShutdown requires CCObject-derived T"
        );
        static auto* const leaked = [] {
            return new std::vector<geode::WeakRef<T>>();
        }();
        leaked->push_back(std::move(ref));
    }
} // namespace luax::detail
