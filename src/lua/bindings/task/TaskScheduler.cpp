#include "TaskScheduler.hpp"

#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>

namespace luax {
    TaskScheduler& TaskScheduler::get() {
        static TaskScheduler s_instance;
        return s_instance;
    }

    std::uint64_t TaskScheduler::add(LuaRef callback, double delaySeconds, double intervalSeconds) {
        if (full()) {
            return 0;
        }
        Task task;
        std::uint64_t const id = m_nextId++;
        task.id = id;
        task.callback = std::move(callback);
        task.remaining = delaySeconds;
        task.interval = intervalSeconds;
        m_timed.insertWithId(id, std::move(task));
        m_deferredIds[id] = false;
        return id;
    }

    std::uint64_t TaskScheduler::addDeferred(LuaRef callback) {
        if (full()) {
            return 0;
        }
        Task task;
        std::uint64_t const id = m_nextId++;
        task.id = id;
        task.callback = std::move(callback);
        m_deferred.insertWithId(id, std::move(task));
        m_deferredIds[id] = true;
        return id;
    }

    void TaskScheduler::cancel(std::uint64_t id) {
        if (auto* task = find(id)) {
            task->cancelled = true;
        }
    }

    TaskScheduler::Task* TaskScheduler::find(std::uint64_t id) {
        auto it = m_deferredIds.find(id);
        if (it == m_deferredIds.end()) {
            return nullptr;
        }
        auto& slots = it->second ? m_deferred : m_timed;
        Task* task = slots.find(id);
        if (!task || task->cancelled) {
            return nullptr;
        }
        return task;
    }

    bool TaskScheduler::fire(Task& task) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return false;
        auto* L = task.callback.luaState();
        if (!L) return false;
        int top = lua_gettop(L);
        if (!task.callback.push()) return false;
        Runtime::ResourcesRootScope scope(*runtime, task.callback.resourcesRoot());
        bool ok = runtime->protectedCall(0, 0, "task", kHookScriptDeadlineMs).isOk();
        lua_settop(L, top);
        return ok;
    }

    void TaskScheduler::fireDeferred() {
        m_deferred.forEachIndexSnapshot([&](std::size_t, Task& task) {
            if (task.cancelled) {
                return;
            }

            (void)fire(task);
            task.cancelled = true;
        });
    }

    void TaskScheduler::fireTimedDue(std::vector<std::size_t> const& due) {
        for (std::size_t i : due) {
            if (i >= m_timed.size()) continue;
            Task& task = m_timed[i];
            if (task.cancelled) continue;

            bool ok = fire(task);
            Task& current = m_timed[i];
            if (!current.cancelled) {
                if (!ok) current.cancelled = true;
                else if (current.interval > 0.0) {
                    current.remaining = current.interval;
                }
                else {
                    current.cancelled = true;
                }
            }
        }
    }

    void TaskScheduler::eraseTaskAt(IndexedSlotMap<Task>& slots, std::size_t index) {
        std::uint64_t const id = slots.idAt(index);
        slots.eraseAt(index);
        m_deferredIds.erase(id);
    }

    void TaskScheduler::compact(IndexedSlotMap<Task>& slots) {
        for (std::size_t i = 0; i < slots.size();) {
            if (!slots[i].cancelled) {
                ++i;
                continue;
            }
            slots[i].callback.reset();
            eraseTaskAt(slots, i);
        }
    }

    void TaskScheduler::advance(double dt, lua_State* L) {
        (void)L;
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return;

        if (!m_deferred.empty()) {
            fireDeferred();
            compact(m_deferred);
        }

        if (m_timed.empty()) return;

        std::vector<std::size_t> due;
        due.reserve(m_timed.size());
        for (std::size_t i = 0; i < m_timed.size(); ++i) {
            Task& task = m_timed[i];
            if (task.cancelled) continue;
            task.remaining -= dt;
            if (task.remaining <= 0.0) {
                due.push_back(i);
            }
        }

        if (!due.empty()) {
            fireTimedDue(due);
            compact(m_timed);
        }
    }

    void TaskScheduler::clear() {
        for (std::size_t i = 0; i < m_timed.size(); ++i) {
            m_timed[i].callback.reset();
        }
        for (std::size_t i = 0; i < m_deferred.size(); ++i) {
            m_deferred[i].callback.reset();
        }
        m_timed.clear();
        m_deferred.clear();
        m_deferredIds.clear();
    }

    bool TaskScheduler::full() const {
        return m_timed.size() + m_deferred.size() >= kMaxScheduledTasks;
    }

    std::size_t TaskScheduler::activeCount() const {
        std::size_t n = 0;
        for (std::size_t i = 0; i < m_timed.size(); ++i) {
            if (!m_timed[i].cancelled) ++n;
        }
        for (std::size_t i = 0; i < m_deferred.size(); ++i) {
            if (!m_deferred[i].cancelled) ++n;
        }
        return n;
    }

#if defined(LUAUAPI_HOST_TESTS)
    bool TaskScheduler::isScheduled(std::uint64_t id) const {
        auto it = m_deferredIds.find(id);
        if (it == m_deferredIds.end()) {
            return false;
        }
        auto const& slots = it->second ? m_deferred : m_timed;
        Task const* task = slots.find(id);
        return task != nullptr && !task->cancelled;
    }
#endif

#if !defined(LUAUAPI_HOST_TESTS)
    namespace {
        class TaskTickNode final : public cocos2d::CCNode {
        public:
            void update(float dt) override {
                auto* runtime = Runtime::getIfInitialized();
                if (!runtime) return;
                auto* L = runtime->state();
                if (!L) return;
                TaskScheduler::get().advance(static_cast<double>(dt), L);
            }
        };

        TaskTickNode* s_tickNode = nullptr;
    } // namespace

    void armTaskTick() {
        if (s_tickNode) return;
        auto* director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;
        auto* scheduler = director->getScheduler();
        if (!scheduler) return;
        s_tickNode = new TaskTickNode();
        s_tickNode->init();
        scheduler->scheduleUpdateForTarget(s_tickNode, 0, false);
    }

    void disarmTaskTick() {
        if (!s_tickNode) return;
        if (auto* director = cocos2d::CCDirector::sharedDirector()) {
            if (auto* scheduler = director->getScheduler()) {
                scheduler->unscheduleUpdateForTarget(s_tickNode);
            }
        }
        s_tickNode->release();
        s_tickNode = nullptr;
    }
#else
    void armTaskTick() {}

    void disarmTaskTick() {}
#endif
} // namespace luax
