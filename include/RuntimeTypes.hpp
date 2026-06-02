#pragma once

namespace imes::luauapi {
    inline constexpr int kDefaultScriptDeadlineMs = 250;

    enum class RuntimeStatus {
        NotReady,
        Ready,
        InitFailed,
        Panicked,
    };
}
