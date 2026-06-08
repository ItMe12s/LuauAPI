#include "ImGuiDrawScheduler.hpp"

#include "lua/Config.hpp"
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
        std::uint64_t const id = m_nextId++;
        cb.id = id;
        cb.callback = std::move(callback);
        m_slots.insertWithId(id, std::move(cb));
        return id;
    }

    void ImGuiDrawScheduler::cancel(std::uint64_t id) {
        if (auto* cb = find(id)) {
            cb->cancelled = true;
        }
    }

    ImGuiDrawScheduler::DrawCb* ImGuiDrawScheduler::find(std::uint64_t id) {
        DrawCb* cb = m_slots.find(id);
        if (!cb || cb->cancelled) {
            return nullptr;
        }
        return cb;
    }

    bool ImGuiDrawScheduler::fire(DrawCb& cb) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return false;
        auto* L = cb.callback.luaState();
        if (!L) return false;
        int top = lua_gettop(L);
        if (!cb.callback.push()) return false;
        Runtime::ResourcesRootScope scope(*runtime, cb.callback.resourcesRoot());
        bool ok = runtime->protectedCall(0, 0, "imgui.draw", kImGuiScriptDeadlineMs).isOk();
        lua_settop(L, top);
        return ok;
    }

    void ImGuiDrawScheduler::compactCancelled() {
        for (std::size_t i = 0; i < m_slots.size();) {
            if (!m_slots[i].cancelled) {
                ++i;
                continue;
            }
            m_slots[i].callback.reset();
            m_slots.eraseAt(i);
        }
    }

    void ImGuiDrawScheduler::drawAll() {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || m_slots.empty()) return;

        m_inFrame = true;

        m_slots.forEachIndexSnapshot([&](std::size_t, DrawCb& cb) {
            if (cb.cancelled) {
                return;
            }
            if (!fire(cb)) {
                cb.cancelled = true;
            }
        });

        m_inFrame = false;
        compactCancelled();
    }

    void ImGuiDrawScheduler::clear() {
        for (std::size_t i = 0; i < m_slots.size(); ++i) {
            m_slots[i].callback.reset();
        }
        m_slots.clear();
    }

    bool ImGuiDrawScheduler::full() const {
        return m_slots.size() >= kMaxImGuiDrawCallbacks;
    }

    std::size_t ImGuiDrawScheduler::activeCount() const {
        std::size_t n = 0;
        for (std::size_t i = 0; i < m_slots.size(); ++i) {
            if (!m_slots[i].cancelled) ++n;
        }
        return n;
    }

    bool ImGuiDrawScheduler::isScheduled(std::uint64_t id) const {
        DrawCb const* cb = m_slots.find(id);
        return cb != nullptr && !cb->cancelled;
    }
} // namespace luax
