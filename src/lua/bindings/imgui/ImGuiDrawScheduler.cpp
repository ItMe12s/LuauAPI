#include "ImGuiDrawScheduler.hpp"

#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

#include <algorithm>

namespace luax {
    ImGuiDrawScheduler& ImGuiDrawScheduler::get() {
        static auto* value = new ImGuiDrawScheduler();
        return *value;
    }

    std::uint64_t ImGuiDrawScheduler::add(LuaRef callback) {
        if (m_callbacks.size() >= kMaxImGuiDrawCallbacks) {
            return 0;
        }
        DrawCb cb;
        cb.id = m_nextId++;
        cb.callback = std::move(callback);
        m_callbacks.push_back(std::move(cb));
        return m_callbacks.back().id;
    }

    void ImGuiDrawScheduler::cancel(std::uint64_t id) {
        if (auto* cb = find(id)) {
            cb->cancelled = true;
        }
    }

    ImGuiDrawScheduler::DrawCb* ImGuiDrawScheduler::find(std::uint64_t id) {
        for (auto& cb : m_callbacks) {
            if (cb.id == id && !cb.cancelled) {
                return &cb;
            }
        }
        return nullptr;
    }

    bool ImGuiDrawScheduler::fire(DrawCb& cb) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return false;
        if (!cb.callback.push()) return false;
        Runtime::ResourcesRootScope scope(*runtime, cb.callback.resourcesRoot());
        return runtime->protectedCall(0, 0, "imgui.draw", kImGuiScriptDeadlineMs);
    }

    void ImGuiDrawScheduler::drawAll() {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || m_callbacks.empty()) return;

        m_inFrame = true;

        std::vector<std::uint64_t> due;
        due.reserve(m_callbacks.size());
        for (auto const& cb : m_callbacks) {
            if (!cb.cancelled) due.push_back(cb.id);
        }

        for (std::uint64_t id : due) {
            DrawCb* cb = find(id);
            if (!cb) continue;
            if (!fire(*cb)) {
                cb = find(id);
                if (cb) cb->cancelled = true;
            }
        }

        m_inFrame = false;

        for (auto& cb : m_callbacks) {
            if (cb.cancelled) cb.callback.reset();
        }
        m_callbacks.erase(
            std::remove_if(m_callbacks.begin(), m_callbacks.end(),
                           [](DrawCb const& c) { return c.cancelled; }),
            m_callbacks.end()
        );
    }

    void ImGuiDrawScheduler::clear() {
        for (auto& cb : m_callbacks) {
            cb.callback.reset();
        }
        m_callbacks.clear();
    }

    bool ImGuiDrawScheduler::full() const {
        return m_callbacks.size() >= kMaxImGuiDrawCallbacks;
    }

    std::size_t ImGuiDrawScheduler::activeCount() const {
        std::size_t n = 0;
        for (auto const& cb : m_callbacks) {
            if (!cb.cancelled) ++n;
        }
        return n;
    }
}
