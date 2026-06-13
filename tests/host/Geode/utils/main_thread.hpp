#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace geode {
    namespace detail {
        inline std::thread::id& registeredMainThreadId() {
            static std::thread::id id;
            return id;
        }

        inline std::mutex& mainThreadQueueMutex() {
            static std::mutex mutex;
            return mutex;
        }

        inline std::deque<std::function<void()>>& mainThreadQueue() {
            static std::deque<std::function<void()>> queue;
            return queue;
        }

        inline bool isMainThreadForQueue() {
            auto const& id = registeredMainThreadId();
            if (id == std::thread::id{}) {
                return true;
            }
            return std::this_thread::get_id() == id;
        }
    } // namespace detail

    template <class Fn>
    void queueInMainThread(Fn&& fn) {
        if (detail::isMainThreadForQueue()) {
            std::forward<Fn>(fn)();
            return;
        }
        std::lock_guard lock(detail::mainThreadQueueMutex());
        detail::mainThreadQueue().emplace_back(std::forward<Fn>(fn));
    }

    namespace test {
        inline void bindMainThreadToCurrent() {
            detail::registeredMainThreadId() = std::this_thread::get_id();
        }

        inline void drainMainThreadQueue() {
            std::deque<std::function<void()>> pending;
            {
                std::lock_guard lock(detail::mainThreadQueueMutex());
                pending.swap(detail::mainThreadQueue());
            }
            for (auto& fn : pending) {
                fn();
            }
        }

        inline void clearMainThreadQueue() {
            std::lock_guard lock(detail::mainThreadQueueMutex());
            detail::mainThreadQueue().clear();
            detail::registeredMainThreadId() = std::thread::id{};
        }
    } // namespace test
} // namespace geode
