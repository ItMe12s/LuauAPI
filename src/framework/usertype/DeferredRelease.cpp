#include "framework/usertype/DeferredRelease.hpp"

#include "core/Runtime.hpp"
#include "framework/callback/LuaTrampolineRegistry.hpp"
#include "framework/lifecycle/Lifecycle.hpp"
#include "framework/usertype/WeakRefShutdown.hpp"

#include <deque>
#include <thread>
#include <utility>
#include <vector>

namespace luax {
    void clearDeferredReleases();

    namespace detail {
        struct OwnedDefer {
            cocos2d::CCObject* ptr;
            geode::WeakRef<cocos2d::CCObject> weak;
        };

        std::deque<geode::WeakRef<cocos2d::CCObject>>& deferredBorrowedReleases() {
            static std::deque<geode::WeakRef<cocos2d::CCObject>> queue;
            return queue;
        }

        std::vector<OwnedDefer>& deferredOwnedReleases() {
            static std::vector<OwnedDefer> queue;
            return queue;
        }

        void disposeBorrowedEntry(geode::WeakRef<cocos2d::CCObject>&& weak) {
            if (Runtime::isShuttingDown()) {
                parkWeakRefForPoolSafety(std::move(weak));
            }
        }

        void disposeOwnedEntry(OwnedDefer& entry, bool logicalRelease) {
            if (entry.ptr) {
                if (logicalRelease && entry.ptr->retainCount() > 1) {
                    entry.ptr->release();
                }
                entry.ptr->release();
            }
            if (Runtime::isShuttingDown()) {
                parkWeakRefForPoolSafety(std::move(entry.weak));
            }
        }

        void drainOwnedBatch(std::vector<OwnedDefer>& owned, bool logicalRelease) {
            for (auto& entry : owned) {
                disposeOwnedEntry(entry, logicalRelease);
            }
        }

        void drainBorrowedBatch(std::deque<geode::WeakRef<cocos2d::CCObject>>& borrowed) {
            for (auto& weak : borrowed) {
                disposeBorrowedEntry(std::move(weak));
            }
        }

        bool& deferredReleaseHookRegistered() {
            static bool registered = false;
            return registered;
        }

        void ensureDeferredReleaseShutdownHook() {
            ensureShutdownHook(deferredReleaseHookRegistered(), &clearDeferredReleases);
        }
    } // namespace detail

    void adoptDeferredReleaseThread() {
        if (!Runtime::isMainThread()) {
            Runtime::setMainThreadId(std::this_thread::get_id());
        }
    }

    void clearDeferredReleases() {
        auto& borrowed = detail::deferredBorrowedReleases();
        detail::drainBorrowedBatch(borrowed);
        borrowed.clear();

        auto& owned = detail::deferredOwnedReleases();
        detail::drainOwnedBatch(owned, false);
        owned.clear();

        detail::deferredReleaseHookRegistered() = false;
    }

    void deferBorrowedRelease(geode::WeakRef<cocos2d::CCObject>&& weak) {
        detail::ensureDeferredReleaseShutdownHook();
        detail::deferredBorrowedReleases().push_back(std::move(weak));
    }

    void deferOwnedRelease(cocos2d::CCObject* obj) {
        if (!obj) return;
        auto& queue = detail::deferredOwnedReleases();
        for (auto const& existing : queue) {
            if (existing.ptr == obj) return;
        }
        auto weak = geode::WeakRef<cocos2d::CCObject>(obj);
        if (!weak.valid()) return;
        detail::ensureDeferredReleaseShutdownHook();
        obj->retain();
        queue.push_back({obj, std::move(weak)});
    }

    void drainDeferredReleases() {
        if (Runtime::isShuttingDown()) {
            clearDeferredReleases();
            return;
        }
        auto& borrowedQueue = detail::deferredBorrowedReleases();
        auto& ownedQueue = detail::deferredOwnedReleases();

        while (!borrowedQueue.empty() || !ownedQueue.empty()) {
            std::vector<detail::OwnedDefer> owned;
            owned.swap(ownedQueue);
            detail::drainOwnedBatch(owned, true);

            std::deque<geode::WeakRef<cocos2d::CCObject>> borrowed;
            borrowed.swap(borrowedQueue);
            detail::drainBorrowedBatch(borrowed);
        }
        drainDeferredTrampolineReleases();
    }
} // namespace luax
