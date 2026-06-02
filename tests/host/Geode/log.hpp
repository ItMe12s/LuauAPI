#pragma once

#include <string_view>
#include <utility>

namespace geode::log {
    template <class... Args>
    inline void debug(std::string_view, Args&&...) {}

    template <class... Args>
    inline void info(std::string_view, Args&&...) {}

    template <class... Args>
    inline void warn(std::string_view, Args&&...) {}

    template <class... Args>
    inline void error(std::string_view, Args&&...) {}
}
