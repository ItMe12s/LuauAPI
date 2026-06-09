#include "ImGuiDrawScheduler.hpp"

#include "lua/Config.hpp"
#include "lua/bindings/framework/ScheduledCallback.hpp"
#include "lua/runtime/Runtime.hpp"

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
        return m_store.insert(std::move(cb));
    }

    void ImGuiDrawScheduler::cancel(std::uint64_t id) {
        m_store.cancel(id);
    }

    ImGuiDrawScheduler::DrawCb* ImGuiDrawScheduler::find(std::uint64_t id) {
        return m_store.find(id);
    }

    bool ImGuiDrawScheduler::fire(DrawCb& cb) {
        return fireProtectedCallback(cb.callback, "imgui.draw", kImGuiScriptDeadlineMs);
    }

    void ImGuiDrawScheduler::drawAll() {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || m_store.empty()) return;

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
        m_store.compactCancelled();
    }

    void ImGuiDrawScheduler::clear() {
        m_store.clear();
    }

    bool ImGuiDrawScheduler::full() const {
        return m_store.full(kMaxImGuiDrawCallbacks);
    }

    std::size_t ImGuiDrawScheduler::activeCount() const {
        return m_store.activeCount();
    }

    bool ImGuiDrawScheduler::isScheduled(std::uint64_t id) const {
        return m_store.isScheduled(id);
    }
} // namespace luax
