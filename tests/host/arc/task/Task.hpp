#pragma once

#include <arc/future/Context.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace arc {
    template <class T>
    struct TaskState {
        std::optional<T> value;
        bool ready = false;
        bool aborted = false;
        bool detached = false;
        std::optional<std::string> error;
    };

    template <>
    struct TaskState<void> {
        bool ready = false;
        bool aborted = false;
        bool detached = false;
        std::optional<std::string> error;
    };

    template <class T = void>
    class TaskHandle {
    public:
        TaskHandle() = default;

        explicit TaskHandle(std::shared_ptr<TaskState<T>> state) : m_state(std::move(state)) {}

        TaskHandle(TaskHandle const&) = delete;
        TaskHandle& operator=(TaskHandle const&) = delete;

        TaskHandle(TaskHandle&&) noexcept = default;
        TaskHandle& operator=(TaskHandle&&) noexcept = default;

        ~TaskHandle() {
            detach();
        }

        std::optional<T> poll(Context&) {
            validate();
            if (m_state->error) {
                auto error = *m_state->error;
                detach();
                throw std::runtime_error(error);
            }
            if (!m_state->ready) {
                return std::nullopt;
            }
            auto out = std::move(m_state->value);
            detach();
            return out;
        }

        void abort() {
            validate();
            m_state->aborted = true;
            detach();
        }

        void detach() noexcept {
            if (m_state) {
                m_state->detached = true;
                m_state.reset();
            }
        }

        void setName(std::string) {}

        bool isValid() const noexcept {
            return m_state != nullptr;
        }

        explicit operator bool() const noexcept {
            return isValid();
        }

    private:
        void validate() const {
            if (!m_state) {
                throw std::runtime_error("Invalid task handle");
            }
        }

        std::shared_ptr<TaskState<T>> m_state;
    };

    template <>
    class TaskHandle<void> {
    public:
        TaskHandle() = default;

        explicit TaskHandle(std::shared_ptr<TaskState<void>> state) : m_state(std::move(state)) {}

        TaskHandle(TaskHandle const&) = delete;
        TaskHandle& operator=(TaskHandle const&) = delete;

        TaskHandle(TaskHandle&&) noexcept = default;
        TaskHandle& operator=(TaskHandle&&) noexcept = default;

        ~TaskHandle() {
            detach();
        }

        bool poll(Context&) {
            validate();
            if (m_state->error) {
                auto error = *m_state->error;
                detach();
                throw std::runtime_error(error);
            }
            if (!m_state->ready) {
                return false;
            }
            detach();
            return true;
        }

        void abort() {
            validate();
            m_state->aborted = true;
            detach();
        }

        void detach() noexcept {
            if (m_state) {
                m_state->detached = true;
                m_state.reset();
            }
        }

        void setName(std::string) {}

        bool isValid() const noexcept {
            return m_state != nullptr;
        }

        explicit operator bool() const noexcept {
            return isValid();
        }

    private:
        void validate() const {
            if (!m_state) {
                throw std::runtime_error("Invalid task handle");
            }
        }

        std::shared_ptr<TaskState<void>> m_state;
    };
} // namespace arc
