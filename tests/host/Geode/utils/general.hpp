#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace geode::utils {
    inline std::string numToString(int value) {
        return std::to_string(value);
    }

    inline float getDisplayFactor() {
        return 1.f;
    }
}
