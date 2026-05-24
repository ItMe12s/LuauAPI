#pragma once

#include <cstddef>

namespace luax {
    constexpr std::size_t kMaxScriptBytes            = 4 * 1024 * 1024;
    constexpr std::size_t kMaxBytecodeCacheEntries   = 512;
    constexpr std::size_t kMinMemoryLimitBytes       = 16 * 1024 * 1024;
    constexpr std::size_t kDefaultMemoryLimitBytes   = 64 * 1024 * 1024;
    constexpr std::size_t kMaxMemoryLimitBytes       = 512 * 1024 * 1024;
    constexpr std::size_t kMaxHookCallbacksGlobal    = 4096;
    constexpr std::size_t kMaxHookCallbacksPerTarget = 64;
    constexpr int kDefaultScriptDeadlineMs           = 250;
}
