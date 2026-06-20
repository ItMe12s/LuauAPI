#include "bindings/imgui/ImGuiDrawScheduler.hpp"

#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/callback/LuaCallback.hpp"

#include <thread>

namespace luax {
    ImGuiDrawScheduler& ImGuiDrawScheduler::get() {
        static ImGuiDrawScheduler s_instance;
        return s_instance;
    }

    std::uint64_t ImGuiDrawScheduler::add(LuaRef callback) {
        if (full()) {
            return 0;
        }
        DrawCb cb;
        cb.callback = std::move(callback);
        return insertSlot(m_store, std::move(cb), m_nextId);
    }

    void ImGuiDrawScheduler::cancel(std::uint64_t id) {
        cancelSlot(m_store, id);
    }

    bool ImGuiDrawScheduler::fire(DrawCb& cb) {
        return LuaCallback::fire(cb.callback, "imgui.draw", kImGuiScriptDeadlineMs);
    }

    void ImGuiDrawScheduler::drawAll() {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || m_store.empty()) return;
#if defined(GEODE_IS_MACOS)
        if (!Runtime::isMainThread()) {
            Runtime::setMainThreadId(std::this_thread::get_id());
        }
#endif

        m_inFrame = true;

        m_store.forEachIndexSnapshot([&](std::size_t, DrawCb& cb) {
            if (cb.cancelled) {
                return;
            }
            if (!fire(cb)) {
                cb.cancelled = true;
            }
        });

        m_inFrame = false;
        compactCancelledSlots(m_store);
    }

    void ImGuiDrawScheduler::clear() {
        clearSlots(m_store);
    }

    bool ImGuiDrawScheduler::full() const {
        return slotMapFull(m_store, kMaxImGuiDrawCallbacks);
    }

    std::size_t ImGuiDrawScheduler::activeCount() const {
        return activeSlotCount(m_store);
    }

    bool ImGuiDrawScheduler::isScheduled(std::uint64_t id) const {
        return isActiveSlot(m_store, id);
    }
} // namespace luax
