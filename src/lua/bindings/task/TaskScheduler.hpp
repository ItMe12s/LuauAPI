#pragma once

#include "lua/bindings/framework/LuaRef.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

struct lua_State;

namespace luax {
    class TaskScheduler {
    public:
        static TaskScheduler& get();

        std::uint64_t add(LuaRef callback, double delaySeconds, double intervalSeconds);
        void cancel(std::uint64_t id);
        void advance(double dt, lua_State* L);
        void clear();

        std::size_t activeCount() const;
        bool full() const;

    private:
        struct Task {
            std::uint64_t id = 0;
            LuaRef callback;
            double remaining = 0.0;
            double interval = 0.0;
            bool cancelled = false;
        };

        Task* find(std::uint64_t id);
        bool fire(Task& task);

        std::vector<Task> m_tasks;
        std::uint64_t m_nextId = 1;
    };

    void armTaskTick();
    void disarmTaskTick();
}
