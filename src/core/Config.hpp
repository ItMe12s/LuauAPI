#pragma once

#include <RuntimeTypes.hpp>
#include <cstddef>

namespace luax {
    // Compilation and Bytecode
    constexpr int kMaxCompileDeadlineMs = 15000;
    constexpr std::size_t kMaxBytecodeCacheEntries = 512;
    constexpr std::size_t kMaxBytecodeCacheBytes = 64 * 1024 * 1024;

    // Filesystem
    constexpr std::size_t kMaxFsReadBytes = 32 * 1024 * 1024;
    constexpr std::size_t kMaxFsWriteBytes = 32 * 1024 * 1024;
    constexpr std::size_t kMaxFsListEntries = 4096;
    constexpr std::size_t kMaxFsListNameBytes = 256 * 1024;

    // Hooks, callbacks, tasks
    constexpr std::size_t kMaxHookCallbacksGlobal = 4096;
    constexpr std::size_t kMaxHookCallbacksPerTarget = 64;
    constexpr std::size_t kMaxScheduledTasks = 4096;
    constexpr std::size_t kMaxCallbackTrampolines = 4096;

    // ImGui
    constexpr std::size_t kMaxImGuiDrawCallbacks = 256;

    // JSON limits
    constexpr std::size_t kMaxJsonParseBytes = 8 * 1024 * 1024;
    constexpr int kMaxJsonDepth = 32;
    constexpr char kJsonDepthExceededMsg[] = "json exceeds maximum depth";

    // Memory
    constexpr std::size_t kMemoryLimitBytes = 512 * 1024 * 1024;

    // Script deadlines for hooks/ImGui
    constexpr int kHookScriptDeadlineMs = 30000;
    constexpr int kImGuiScriptDeadlineMs = 500;

    // Script limits
    inline constexpr int kDefaultScriptDeadlineMs = imes::luauapi::kDefaultScriptDeadlineMs;
    constexpr std::size_t kMaxScriptBytes = 4 * 1024 * 1024;

    // Web limits
    constexpr std::size_t kMaxWebResponseBytes = 32 * 1024 * 1024;
    constexpr std::size_t kMaxWebRequestBytes = 32 * 1024 * 1024;
    constexpr std::size_t kMaxWebConcurrentRequests = 16;

    // WebSocket
    constexpr std::size_t kMaxWebSocketConnections = 16;
    constexpr std::size_t kMaxWebSocketServers = 2;
    constexpr std::size_t kMaxWebSocketServerClients = 32;
    constexpr std::size_t kMaxWebSocketSendBytes = 8 * 1024 * 1024;
    constexpr std::size_t kMaxWebSocketReceiveBytes = 8 * 1024 * 1024;
} // namespace luax
