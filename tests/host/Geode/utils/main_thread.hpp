#pragma once

#include <utility>

namespace geode {
    template <class Fn>
    void queueInMainThread(Fn&& fn) {
        std::forward<Fn>(fn)();
    }
}
