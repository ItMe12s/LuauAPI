#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

namespace geode::utils {
    template <class T>
        requires std::is_arithmetic_v<T>
    inline std::string numToString(T value) {
        return std::to_string(value);
    }

    inline float getDisplayFactor() {
        return 1.f;
    }
} // namespace geode::utils
