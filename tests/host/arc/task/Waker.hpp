#pragma once

namespace arc {
    struct Waker {
        static Waker noop() {
            return {};
        }
    };
} // namespace arc
