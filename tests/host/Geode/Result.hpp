#pragma once

#include <optional>
#include <string>
#include <utility>

namespace geode {
    template <class T>
    class Result {
    public:
        static Result ok(T value) {
            Result result;
            result.m_value = std::move(value);
            return result;
        }

        static Result err(std::string message) {
            Result result;
            result.m_error = std::move(message);
            return result;
        }

        bool isOk() const { return m_value.has_value(); }
        bool isErr() const { return !isOk(); }

        T& unwrap() { return *m_value; }
        T const& unwrap() const { return *m_value; }
        std::string const& unwrapErr() const { return m_error; }

        T unwrapOr(T fallback) const {
            return m_value ? *m_value : std::move(fallback);
        }

    private:
        std::optional<T> m_value;
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

        bool isOk() const { return m_ok; }
        bool isErr() const { return !m_ok; }

        void unwrap() const {}
        std::string const& unwrapErr() const { return m_error; }

    private:
        bool m_ok = true;
        std::string m_error;
    };

    template <class T>
    inline Result<T> Ok(T value) {
        return Result<T>::ok(std::move(value));
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
}
