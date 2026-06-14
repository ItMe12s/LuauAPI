#pragma once

namespace imes::luauapi {
    inline constexpr int kDefaultScriptDeadlineMs = 15000;

    enum class RuntimeStatus {
        NotReady,
        Ready,
        InitFailed,
        Panicked,
    };
} // namespace imes::luauapi
