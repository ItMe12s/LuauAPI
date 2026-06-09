#pragma once

#include "lua/bindings/framework/LuaRef.hpp"
#include "lua/bindings/framework/ScheduledSlotStore.hpp"

#include <cstddef>
#include <cstdint>

namespace luax {
    class ImGuiDrawScheduler final {
    public:
        static ImGuiDrawScheduler& get();

        std::uint64_t add(LuaRef callback);
        void cancel(std::uint64_t id);
        void drawAll();
        void clear();

        std::size_t activeCount() const;
        bool isScheduled(std::uint64_t id) const;
        bool full() const;

        bool inFrame() const {
            return m_inFrame;
        }

    private:
        struct DrawCb {
            std::uint64_t id = 0;
            LuaRef callback;
            bool cancelled = false;
        };

        bool fire(DrawCb& cb);

        ScheduledSlotStore<DrawCb> m_store;
        bool m_inFrame = false;
    };
} // namespace luax
