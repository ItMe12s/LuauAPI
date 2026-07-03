#pragma once

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace geode {
    namespace impl {
        template <class U>
        struct OkHolder {
            U value;
        };
    } // namespace impl

    template <class T>
    class Result {
        using Stored = std::conditional_t<
            std::is_reference_v<T>, std::reference_wrapper<std::remove_reference_t<T>>, T>;
        using Bare = std::remove_reference_t<T>;

    public:
        Result() = default;

        template <class U>
        Result(impl::OkHolder<U> holder) : m_value(static_cast<Stored>(std::move(holder.value))) {}

        static Result err(std::string message) {
            Result result;
            result.m_error = std::move(message);
            return result;
        }

        bool isOk() const {
            return m_value.has_value();
        }

        bool isErr() const {
            return !isOk();
        }

        Bare& unwrap() {
            return ref();
        }

        Bare const& unwrap() const {
            return ref();
        }

        std::string const& unwrapErr() const {
            return m_error;
        }

        T unwrapOr(T fallback) const {
            return m_value ? *m_value : std::move(fallback);
        }

    private:
        Bare& ref() {
            if constexpr (std::is_reference_v<T>) return m_value->get();
            else return *m_value;
        }

        Bare const& ref() const {
            if constexpr (std::is_reference_v<T>) return m_value->get();
            else return *m_value;
        }

        std::optional<Stored> m_value;
        std::string m_error;
    };

    template <>
    class Result<void> {
    public:
        static Result ok() {
            return Result{};
        }

        static Result err(std::string message) {
            Result result;
            result.m_ok = false;
            result.m_error = std::move(message);
            return result;
        }

        bool isOk() const {
            return m_ok;
        }

        bool isErr() const {
            return !m_ok;
        }

        void unwrap() const {}

        std::string const& unwrapErr() const {
            return m_error;
        }

    private:
        bool m_ok = true;
        std::string m_error;
    };

    template <class U>
    inline impl::OkHolder<std::decay_t<U>> Ok(U&& value) {
        return {std::forward<U>(value)};
    }

    inline Result<void> Ok() {
        return Result<void>::ok();
    }

    struct ErrMaker {
        std::string message;

        template <class T>
        operator Result<T>() const {
            return Result<T>::err(message);
        }

        operator Result<void>() const {
            return Result<void>::err(message);
        }
    };

    inline ErrMaker Err(std::string message) {
        return {std::move(message)};
    }
} // namespace geode
