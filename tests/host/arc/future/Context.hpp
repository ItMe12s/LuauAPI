#pragma once

#include <arc/task/Waker.hpp>

namespace arc {
    class Context {
    public:
        explicit Context(Waker* waker) : m_waker(waker) {}

    private:
        Waker* m_waker = nullptr;
    };
} // namespace arc
