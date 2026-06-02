#pragma once

#include <RuntimeTypes.hpp>

#include <cstddef>

namespace luax {
    inline constexpr int kDefaultScriptDeadlineMs = imes::luauapi::kDefaultScriptDeadlineMs;
    constexpr std::size_t kMaxScriptBytes            = 4 * 1024 * 1024;
    constexpr std::size_t kMaxFsReadBytes            = 32 * 1024 * 1024;
    constexpr std::size_t kMaxBytecodeCacheEntries   = 512;
    constexpr std::size_t kMemoryLimitBytes          = 512 * 1024 * 1024;
    constexpr std::size_t kMaxHookCallbacksGlobal    = 4096;
    constexpr std::size_t kMaxHookCallbacksPerTarget = 64;
    constexpr std::size_t kMaxScheduledTasks         = 4096;
    constexpr std::size_t kMaxImGuiDrawCallbacks     = 256;
    constexpr std::size_t kMaxCallbackTrampolines    = 4096;
    constexpr int kHookScriptDeadlineMs              = 50;
    constexpr int kImGuiScriptDeadlineMs             = 16;
}
