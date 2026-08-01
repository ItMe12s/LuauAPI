#pragma once

#include <chrono>
#include <cstdint>

namespace geode::utils {
    template <class Clock = std::chrono::high_resolution_clock>
    class Timer {
    public:
        Timer() : m_start(Clock::now()) {}

        template <class Duration = std::chrono::milliseconds>
        std::int64_t elapsed() const {
            return std::chrono::duration_cast<Duration>(Clock::now() - m_start).count();
        }

    private:
        std::chrono::time_point<Clock> m_start;
    };
} // namespace geode::utils
