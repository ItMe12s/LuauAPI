#pragma once

#include "core/IndexedSlotMap.hpp"
#include "framework/schedule/CancellableSlots.hpp"
#include "framework/usertype/LuaRef.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

struct lua_State;

namespace luax {
    class TaskScheduler final {
    public:
        static TaskScheduler& get();

        std::uint64_t add(
            LuaRef callback, double delaySeconds, double intervalSeconds, bool isThread = false
        );
        std::uint64_t addDeferred(LuaRef callback);

        std::uint64_t addWait(LuaRef thread, double seconds) {
            return add(std::move(thread), seconds, 0.0, true);
        }

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
            double elapsed = 0.0;
            bool cancelled = false;
            bool isThread = false;
        };

        bool fire(Task& task);
        void fireDeferred();
        void fireTimedDue(std::vector<std::size_t> const& due);
        void compact(IndexedSlotMap<Task>& store);

        IndexedSlotMap<Task> m_timed;
        IndexedSlotMap<Task> m_deferred;
        std::uint64_t m_nextId = 1;
    };

    bool armTaskTick();
    void disarmTaskTick();
} // namespace luax
