#pragma once

#include "lua/bindings/framework/LuaRef.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

struct lua_State;

namespace luax {
    class TaskScheduler {
    public:
        static TaskScheduler& get();

        std::uint64_t add(LuaRef callback, double delaySeconds, double intervalSeconds);
        std::uint64_t addDeferred(LuaRef callback);
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

        struct TaskLoc {
            bool deferred = false;
            std::size_t index = 0;
        };

        Task* find(std::uint64_t id);
        bool fire(Task& task);
        void fireDeferred();
        void fireTimedDue(std::vector<std::size_t> const& due);
        void compact(std::vector<Task>& tasks, bool deferred);
        void eraseAt(std::vector<Task>& tasks, bool deferred, std::size_t index);

        std::vector<Task> m_timed;
        std::vector<Task> m_deferred;
        std::unordered_map<std::uint64_t, TaskLoc> m_index;
        std::uint64_t m_nextId = 1;
    };

    void armTaskTick();
    void disarmTaskTick();
}
