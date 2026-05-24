#pragma once

#include <cstddef>

namespace luax {
    constexpr std::size_t kMaxScriptBytes = 4 * 1024 * 1024;
    constexpr std::size_t kMaxBytecodeCacheEntries = 512;
    constexpr std::size_t kDefaultMemoryLimitBytes = 64 * 1024 * 1024;
}
