#pragma once

#include "lua/bindings/framework/LuaRef.hpp"
#include "lua/util/IndexedSlotMap.hpp"

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

        DrawCb* find(std::uint64_t id);
        bool fire(DrawCb& cb);
        void compactCancelled();

        IndexedSlotMap<DrawCb> m_slots;
        std::uint64_t m_nextId = 1;
        bool m_inFrame = false;
    };
} // namespace luax
