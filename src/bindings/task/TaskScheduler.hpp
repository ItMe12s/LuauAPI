#pragma once

#include "framework/usertype/LuaRef.hpp"
#include "framework/schedule/ScheduledSlotStore.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

struct lua_State;

namespace luax {
    class TaskScheduler final {
    public:
        static TaskScheduler& get();

        std::uint64_t add(LuaRef callback, double delaySeconds, double intervalSeconds);
        std::uint64_t addDeferred(LuaRef callback);
        void cancel(std::uint64_t id);
        void advance(double dt, lua_State* L);
        void clear();

        std::size_t activeCount() const;
        bool full() const;

#if defined(LUAUAPI_HOST_TESTS)
        bool isScheduled(std::uint64_t id) const;
#endif

    private:
        struct Task {
            std::uint64_t id = 0;
            LuaRef callback;
            double remaining = 0.0;
            double interval = 0.0;
            bool cancelled = false;
        };

        bool fire(Task& task);
        void fireDeferred();
        void fireTimedDue(std::vector<std::size_t> const& due);
        void compact(ScheduledSlotStore<Task>& store);
        void eraseTaskAt(ScheduledSlotStore<Task>& store, std::size_t index);

        ScheduledSlotStore<Task> m_timed;
        ScheduledSlotStore<Task> m_deferred;
        std::unordered_map<std::uint64_t, bool> m_deferredIds;
        std::uint64_t m_nextId = 1;
    };

    bool armTaskTick();
    void disarmTaskTick();
} // namespace luax
