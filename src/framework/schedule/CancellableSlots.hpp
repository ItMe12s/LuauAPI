#pragma once

#include "core/IndexedSlotMap.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace luax {
    template <typename Entry>
    concept CancellableSlotEntry = HasSlotId<Entry> && requires(Entry& entry) {
        { entry.cancelled } -> std::same_as<bool&>;
        { entry.callback.reset() } -> std::same_as<void>;
    };

    template <CancellableSlotEntry Entry>
    inline void cancelSlot(IndexedSlotMap<Entry>& slots, std::uint64_t id) {
        if (auto* entry = slots.find(id)) {
            entry->cancelled = true;
        }
    }

    template <CancellableSlotEntry Entry>
    inline Entry* findActiveSlot(IndexedSlotMap<Entry>& slots, std::uint64_t id) {
        Entry* entry = slots.find(id);
        if (!entry || entry->cancelled) {
            return nullptr;
        }
        return entry;
    }

    template <CancellableSlotEntry Entry>
    inline Entry const* findActiveSlot(IndexedSlotMap<Entry> const& slots, std::uint64_t id) {
        return findActiveSlot(const_cast<IndexedSlotMap<Entry>&>(slots), id);
    }

    template <CancellableSlotEntry Entry>
    inline void compactCancelledSlots(IndexedSlotMap<Entry>& slots) {
        for (std::size_t i = 0; i < slots.size();) {
            if (!slots[i].cancelled) {
                ++i;
                continue;
            }
            slots[i].callback.reset();
            slots.eraseAt(i);
        }
    }

    template <CancellableSlotEntry Entry>
    inline std::size_t activeSlotCount(IndexedSlotMap<Entry> const& slots) {
        std::size_t n = 0;
        for (std::size_t i = 0; i < slots.size(); ++i) {
            if (!slots[i].cancelled) {
                ++n;
            }
        }
        return n;
    }

    template <CancellableSlotEntry Entry>
    inline bool slotMapFull(IndexedSlotMap<Entry> const& slots, std::size_t maxSlots) {
        return activeSlotCount(slots) >= maxSlots;
    }

    template <CancellableSlotEntry Entry>
    inline bool isActiveSlot(IndexedSlotMap<Entry> const& slots, std::uint64_t id) {
        return findActiveSlot(slots, id) != nullptr;
    }

    template <CancellableSlotEntry Entry>
    inline std::uint64_t insertSlot(IndexedSlotMap<Entry>& slots, Entry value, std::uint64_t& nextId) {
        return slots.insertWithId(nextId++, std::move(value));
    }

    template <CancellableSlotEntry Entry>
    inline void clearSlots(IndexedSlotMap<Entry>& slots) {
        for (std::size_t i = 0; i < slots.size(); ++i) {
            slots[i].callback.reset();
        }
        slots.clear();
    }
} // namespace luax
