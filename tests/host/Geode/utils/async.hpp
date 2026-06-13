#pragma once

#include <Geode/utils/main_thread.hpp>

#include <functional>
#include <optional>
#include <string>
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

        TaskHolder(TaskHolder&& other) noexcept {
            *this = std::move(other);
        }

        TaskHolder& operator=(TaskHolder&& other) noexcept {
            if (this != &other) {
                cancel();
                m_pending = other.m_pending;
                m_cancelled = other.m_cancelled;
                other.m_pending = false;
                other.m_cancelled = true;
            }
            return *this;
        }

        ~TaskHolder() {
            cancel();
        }

        template <class F, class Cb>
        void spawn(F&& future, Cb&& cb) {
            spawn("", std::forward<F>(future), std::forward<Cb>(cb));
        }

        template <class F, class Cb>
        void spawn(std::string name, F&& future, Cb&& cb) {
            (void)name;
            cancel();
            m_cancelled = false;
            m_pending = true;

            if (m_cancelled) {
                m_pending = false;
                return;
            }

            Ret result = std::forward<F>(future).resolve();
            if (m_cancelled) {
                m_pending = false;
                return;
            }

            geode::queueInMainThread([this, cb = std::forward<Cb>(cb), result = std::move(result)]() mutable {
                if (m_cancelled) {
                    m_pending = false;
                    return;
                }
                if constexpr (std::is_void_v<Ret>) {
                    cb();
                }
                else {
                    cb(std::move(result));
                }
                m_pending = false;
            });
        }

        void cancel() {
            m_cancelled = true;
            m_pending = false;
        }

        void setName(std::string /*name*/) {}

        bool isPending() const {
            return m_pending;
        }

    private:
        bool m_pending = false;
        bool m_cancelled = false;
    };
} // namespace geode::async
