#pragma once

#include <exception>

namespace geode::utils {
    [[noreturn]] inline void unreachable() {
        std::terminate();
    }
} // namespace geode::utils
