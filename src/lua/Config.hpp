#pragma once

#include <cstddef>

namespace luax {
    constexpr std::size_t kMaxScriptBytes            = 4 * 1024 * 1024;
    constexpr std::size_t kMaxBytecodeCacheEntries   = 512;
    constexpr std::size_t kMemoryLimitBytes          = 512 * 1024 * 1024;
    constexpr std::size_t kMaxHookCallbacksGlobal    = 4096;
    constexpr std::size_t kMaxHookCallbacksPerTarget = 64;
    constexpr std::size_t kMaxScheduledTasks         = 4096;
    constexpr int kDefaultScriptDeadlineMs           = 250;
    constexpr int kHookScriptDeadlineMs              = 50;
}
