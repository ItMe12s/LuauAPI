#pragma once

#include <Geode/Result.hpp>

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace geode::async {
    template <class T, class Fn>
    struct MainThreadAwaiter {
        Fn fn;
        std::optional<T> result;

        bool await_ready() {
            result = fn();
            return true;
        }

        std::optional<T> await_resume() {
            return std::move(result);
        }

        void await_suspend(...) noexcept {}
    };

    template <class T, class Fn>
    MainThreadAwaiter<T, std::decay_t<Fn>> waitForMainThread(Fn&& fn) {
        return {std::forward<Fn>(fn), {}};
    }

    template <typename Ret = void>
    class TaskHolder {
    public:
        TaskHolder() = default;
        TaskHolder(TaskHolder const&) = delete;
        TaskHolder& operator=(TaskHolder const&) = delete;
        TaskHolder(TaskHolder&&) noexcept = default;
        TaskHolder& operator=(TaskHolder&&) noexcept = default;

        ~TaskHolder() { cancel(); }

        void cancel() {}
    };
}
