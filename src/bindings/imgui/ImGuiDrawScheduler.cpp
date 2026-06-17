#include "bindings/imgui/ImGuiDrawScheduler.hpp"

#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/schedule/ScheduledCallback.hpp"

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
        return m_store.insert(std::move(cb));
    }

    void ImGuiDrawScheduler::cancel(std::uint64_t id) {
        m_store.cancel(id);
    }

    bool ImGuiDrawScheduler::fire(DrawCb& cb) {
        return fireProtectedCallback(cb.callback, "imgui.draw", kImGuiScriptDeadlineMs);
    }

    void ImGuiDrawScheduler::drawAll() {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || m_store.empty()) return;
#if defined(GEODE_IS_MACOS)
        if (!Runtime::isMainThread()) {
            // #region agent log
            Runtime::debugThreadProbe(
                "post-fix",
                "H11,H12,H13",
                "src/bindings/imgui/ImGuiDrawScheduler.cpp:drawAll",
                "adopting ImGui draw thread before callbacks"
            );
            // #endregion
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
