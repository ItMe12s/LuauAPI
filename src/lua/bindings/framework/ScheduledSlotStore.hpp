#pragma once

#include "lua/bindings/framework/LuaRef.hpp"
#include "lua/util/IndexedSlotMap.hpp"

#include <cstddef>
#include <cstdint>

namespace luax {
    template <typename Entry>
    concept CancellableSlotEntry = HasSlotId<Entry> && requires(Entry& entry) {
        { entry.cancelled } -> std::same_as<bool&>;
        { entry.callback.reset() } -> std::same_as<void>;
    };

    template <CancellableSlotEntry Entry>
    class ScheduledSlotStore {
    public:
        std::uint64_t insert(Entry value) {
            return insertWithId(m_nextId++, std::move(value));
        }

        std::uint64_t insertWithId(std::uint64_t id, Entry value) {
            m_slots.insertWithId(id, std::move(value));
            return id;
        }

        void cancel(std::uint64_t id) {
            if (auto* entry = find(id)) {
                entry->cancelled = true;
            }
        }

        Entry* find(std::uint64_t id) {
            Entry* entry = m_slots.find(id);
            if (!entry || entry->cancelled) {
                return nullptr;
            }
            return entry;
        }

        Entry const* find(std::uint64_t id) const {
            Entry const* entry = m_slots.find(id);
            if (!entry || entry->cancelled) {
                return nullptr;
            }
            return entry;
        }

        void compactCancelled() {
            for (std::size_t i = 0; i < m_slots.size();) {
                if (!m_slots[i].cancelled) {
                    ++i;
                    continue;
                }
                m_slots[i].callback.reset();
                m_slots.eraseAt(i);
            }
        }

        void clear() {
            for (std::size_t i = 0; i < m_slots.size(); ++i) {
                m_slots[i].callback.reset();
            }
            m_slots.clear();
        }

        bool full(std::size_t maxSlots) const {
            return m_slots.size() >= maxSlots;
        }

        std::size_t activeCount() const {
            std::size_t n = 0;
            for (std::size_t i = 0; i < m_slots.size(); ++i) {
                if (!m_slots[i].cancelled) {
                    ++n;
                }
            }
            return n;
        }

        bool isScheduled(std::uint64_t id) const {
            return find(id) != nullptr;
        }

        std::size_t size() const {
            return m_slots.size();
        }

        bool empty() const {
            return m_slots.empty();
        }

        Entry& operator[](std::size_t index) {
            return m_slots[index];
        }

        Entry const& operator[](std::size_t index) const {
            return m_slots[index];
        }

        template <typename Fn>
        void forEachIndexSnapshot(Fn&& fn) {
            m_slots.forEachIndexSnapshot(std::forward<Fn>(fn));
        }

        IndexedSlotMap<Entry>& slots() {
            return m_slots;
        }

        IndexedSlotMap<Entry> const& slots() const {
            return m_slots;
        }

    private:
        IndexedSlotMap<Entry> m_slots;
        std::uint64_t m_nextId = 1;
    };
} // namespace luax
