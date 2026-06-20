#pragma once

#include "core/Runtime.hpp"
#include "framework/lifecycle/Lifecycle.hpp"
#include "framework/usertype/WeakRefShutdown.hpp"

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <deque>
#include <utility>
#include <vector>

namespace luax {
    namespace detail {
        struct OwnedDefer {
            cocos2d::CCObject* ptr;
            geode::WeakRef<cocos2d::CCObject> weak;
        };

        inline std::deque<geode::WeakRef<cocos2d::CCObject>>& deferredBorrowedReleases() {
            static std::deque<geode::WeakRef<cocos2d::CCObject>> queue;
            return queue;
        }

        inline std::vector<OwnedDefer>& deferredOwnedReleases() {
            static std::vector<OwnedDefer> queue;
            return queue;
        }
    } // namespace detail

    inline void clearDeferredReleases() {
        auto& borrowed = detail::deferredBorrowedReleases();
        for (auto& weak : borrowed) {
            detail::leakWeakRefDuringShutdown(std::move(weak));
        }
        borrowed.clear();
        auto& owned = detail::deferredOwnedReleases();
        for (auto& entry : owned) {
            detail::leakWeakRefDuringShutdown(std::move(entry.weak));
        }
        owned.clear();
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
        auto& queue = detail::deferredOwnedReleases();
        for (auto const& existing : queue) {
            if (existing.ptr == obj) return;
        }
        auto weak = geode::WeakRef<cocos2d::CCObject>(obj);
        if (!weak.valid()) return;
        ensureDeferredReleaseShutdownHook();
        queue.push_back({obj, std::move(weak)});
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
            std::vector<detail::OwnedDefer> owned;
            owned.swap(ownedQueue);

            for (auto& entry : owned) {
                if (entry.ptr && entry.weak.valid()) {
                    entry.ptr->release();
                }
            }
            borrowed.clear();
        }
    }
} // namespace luax
