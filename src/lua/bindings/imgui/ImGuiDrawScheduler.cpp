#include "ImGuiDrawScheduler.hpp"

#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

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
        m_index[cb.id] = m_callbacks.size();
        m_callbacks.push_back(std::move(cb));
        return m_callbacks.back().id;
    }

    void ImGuiDrawScheduler::cancel(std::uint64_t id) {
        if (auto* cb = find(id)) {
            cb->cancelled = true;
        }
    }

    ImGuiDrawScheduler::DrawCb* ImGuiDrawScheduler::find(std::uint64_t id) {
        auto it = m_index.find(id);
        if (it == m_index.end()) {
            return nullptr;
        }
        if (it->second >= m_callbacks.size()) {
            return nullptr;
        }
        DrawCb& cb = m_callbacks[it->second];
        if (cb.id != id || cb.cancelled) {
            return nullptr;
        }
        return &cb;
    }

    bool ImGuiDrawScheduler::fire(DrawCb& cb) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return false;
        auto* L = cb.callback.luaState();
        if (!L) return false;
        int top = lua_gettop(L);
        if (!cb.callback.push()) return false;
        Runtime::ResourcesRootScope scope(*runtime, cb.callback.resourcesRoot());
        bool ok = runtime->protectedCall(0, 0, "imgui.draw", kImGuiScriptDeadlineMs);
        lua_settop(L, top);
        return ok;
    }

    void ImGuiDrawScheduler::eraseAt(std::size_t index) {
        std::size_t last = m_callbacks.size() - 1;
        m_index.erase(m_callbacks[index].id);
        if (index != last) {
            m_callbacks[index] = std::move(m_callbacks[last]);
            m_index[m_callbacks[index].id] = index;
        }
        m_callbacks.pop_back();
    }

    void ImGuiDrawScheduler::drawAll() {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || m_callbacks.empty()) return;

        m_inFrame = true;

        for (std::size_t i = 0; i < m_callbacks.size(); ++i) {
            DrawCb& cb = m_callbacks[i];
            if (cb.cancelled) continue;
            if (!fire(cb)) {
                cb.cancelled = true;
            }
        }

        m_inFrame = false;

        for (std::size_t i = 0; i < m_callbacks.size();) {
            if (!m_callbacks[i].cancelled) {
                ++i;
                continue;
            }
            m_callbacks[i].callback.reset();
            eraseAt(i);
        }
    }

    void ImGuiDrawScheduler::clear() {
        for (auto& cb : m_callbacks) {
            cb.callback.reset();
        }
        m_callbacks.clear();
        m_index.clear();
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
