#pragma once

#include <fmt/format.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace geode::log {
    inline std::vector<std::string>& capturedInfoMessages() {
        static std::vector<std::string> messages;
        return messages;
    }

    inline std::vector<std::string>& capturedErrorMessages() {
        static std::vector<std::string> messages;
        return messages;
    }

    inline void clearCapturedMessages() {
        capturedInfoMessages().clear();
        capturedErrorMessages().clear();
    }

    template <class... Args>
    inline std::string formatCapturedMessage(std::string_view pattern, Args&&... args) {
        if constexpr (sizeof...(Args) == 0) {
            return std::string(pattern);
        }
        else {
            return fmt::format(fmt::runtime(pattern), std::forward<Args>(args)...);
        }
    }

    template <class... Args>
    inline void debug(std::string_view, Args&&...) {}

    template <class... Args>
    inline void info(std::string_view pattern, Args&&... args) {
        capturedInfoMessages().push_back(formatCapturedMessage(pattern, std::forward<Args>(args)...));
    }

    template <class... Args>
    inline void warn(std::string_view, Args&&...) {}

    template <class... Args>
    inline void error(std::string_view pattern, Args&&... args) {
        capturedErrorMessages().push_back(formatCapturedMessage(pattern, std::forward<Args>(args)...));
    }
}
