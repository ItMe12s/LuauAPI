#pragma once

#include "core/IndexedSlotMap.hpp"
#include "framework/schedule/CancellableSlots.hpp"
#include "framework/usertype/LuaRef.hpp"

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

        IndexedSlotMap<DrawCb> m_store;
        std::uint64_t m_nextId = 1;
        bool m_inFrame = false;
    };
} // namespace luax
