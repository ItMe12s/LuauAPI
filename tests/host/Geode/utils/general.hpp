#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace geode::utils {
    inline std::string numToString(int value) {
        return std::to_string(value);
    }

    inline std::uint64_t hash(std::string_view value) {
        return static_cast<std::uint64_t>(std::hash<std::string_view>{}(value));
    }
}
