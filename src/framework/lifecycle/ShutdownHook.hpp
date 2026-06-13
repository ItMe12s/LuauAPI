#pragma once

#include "core/Runtime.hpp"

namespace luax {
    inline void ensureShutdownHook(bool& registered, void (*clearFn)()) {
        if (registered) return;
        auto* rt = Runtime::getIfInitialized();
        if (!rt) return;
        rt->registerShutdownHook(clearFn);
        registered = true;
    }
} // namespace luax
