#pragma once

#include "core/Runtime.hpp"
#include "framework/lifecycle/ShutdownHook.hpp"
#include "framework/usertype/WeakRefShutdown.hpp"

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <deque>
#include <utility>
#include <vector>

namespace luax {
    namespace detail {
        inline std::deque<geode::WeakRef<cocos2d::CCObject>>& deferredBorrowedReleases() {
            static std::deque<geode::WeakRef<cocos2d::CCObject>> queue;
            return queue;
        }

        inline std::vector<cocos2d::CCObject*>& deferredOwnedReleases() {
            static std::vector<cocos2d::CCObject*> queue;
            return queue;
        }
    } // namespace detail

    inline void clearDeferredReleases() {
        auto& borrowed = detail::deferredBorrowedReleases();
        for (auto& weak : borrowed) {
            detail::leakWeakRefDuringShutdown(std::move(weak));
        }
        borrowed.clear();
        detail::deferredOwnedReleases().clear();
    }

    inline void ensureDeferredReleaseShutdownHook() {
        static bool registered = false;
        ensureShutdownHook(registered, &clearDeferredReleases);
    }

    inline void deferBorrowedRelease(geode::WeakRef<cocos2d::CCObject>&& weak) {
        ensureDeferredReleaseShutdownHook();
        detail::deferredBorrowedReleases().push_back(std::move(weak));
    }

    inline void deferOwnedRelease(cocos2d::CCObject* obj) {
        if (!obj) return;
        ensureDeferredReleaseShutdownHook();
        detail::deferredOwnedReleases().push_back(obj);
    }

    inline void drainDeferredReleases() {
        if (Runtime::isShuttingDown()) {
            clearDeferredReleases();
            return;
        }
        auto& borrowedQueue = detail::deferredBorrowedReleases();
        auto& ownedQueue = detail::deferredOwnedReleases();

        while (!borrowedQueue.empty() || !ownedQueue.empty()) {
            std::deque<geode::WeakRef<cocos2d::CCObject>> borrowed;
            borrowed.swap(borrowedQueue);
            std::vector<cocos2d::CCObject*> owned;
            owned.swap(ownedQueue);

            for (auto* obj : owned) {
                if (obj && geode::detail::isLiveCocosObject(obj)) {
                    obj->release();
                }
            }
            borrowed.clear();
        }
    }
} // namespace luax
