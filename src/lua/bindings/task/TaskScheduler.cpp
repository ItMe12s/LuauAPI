#include "TaskScheduler.hpp"

#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

#include <algorithm>

#include <Geode/Geode.hpp>

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
        task.id = m_nextId++;
        task.callback = std::move(callback);
        task.remaining = delaySeconds;
        task.interval = intervalSeconds;
        m_index[task.id] = TaskLoc{false, m_timed.size()};
        m_timed.push_back(std::move(task));
        return m_timed.back().id;
    }

    std::uint64_t TaskScheduler::addDeferred(LuaRef callback) {
        if (full()) {
            return 0;
        }
        Task task;
        task.id = m_nextId++;
        task.callback = std::move(callback);
        m_index[task.id] = TaskLoc{true, m_deferred.size()};
        m_deferred.push_back(std::move(task));
        return m_deferred.back().id;
    }

    void TaskScheduler::cancel(std::uint64_t id) {
        if (auto* task = find(id)) {
            task->cancelled = true;
        }
    }

    TaskScheduler::Task* TaskScheduler::find(std::uint64_t id) {
        auto it = m_index.find(id);
        if (it == m_index.end()) {
            return nullptr;
        }
        auto& loc = it->second;
        auto& tasks = loc.deferred ? m_deferred : m_timed;
        if (loc.index >= tasks.size()) {
            return nullptr;
        }
        Task& task = tasks[loc.index];
        if (task.id != id || task.cancelled)
            return nullptr;
        return &task;
    }

    bool TaskScheduler::fire(Task& task) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return false;
        auto* L = task.callback.luaState();
        if (!L) return false;
        int top = lua_gettop(L);
        if (!task.callback.push()) return false;
        Runtime::ResourcesRootScope scope(*runtime, task.callback.resourcesRoot());
        bool ok = runtime->protectedCall(0, 0, "task", kHookScriptDeadlineMs);
        lua_settop(L, top);
        return ok;
    }

    void TaskScheduler::fireDeferred() {
        std::vector<std::size_t> due;
        due.reserve(m_deferred.size());
        for (std::size_t i = 0; i < m_deferred.size(); ++i) {
            if (!m_deferred[i].cancelled) {
                due.push_back(i);
            }
        }

        for (std::size_t i : due) {
            if (i >= m_deferred.size()) continue;
            Task& task = m_deferred[i];
            if (task.cancelled) continue;

            bool ok = fire(task);
            (void)ok;
            m_deferred[i].cancelled = true;
        }
    }

    void TaskScheduler::fireTimedDue(std::vector<std::size_t> const& due) {
        for (std::size_t i : due) {
            if (i >= m_timed.size()) continue;
            Task& task = m_timed[i];
            if (task.cancelled) continue;

            bool ok = fire(task);
            Task& current = m_timed[i];
            if (!current.cancelled) {
                if (!ok)
                    current.cancelled = true;
                else if (current.interval > 0.0) {
                    current.remaining = current.interval;
                } else {
                    current.cancelled = true;
                }
            }
        }
    }

    void TaskScheduler::eraseAt(std::vector<Task>& tasks, bool deferred, std::size_t index) {
        std::size_t last = tasks.size() - 1;
        m_index.erase(tasks[index].id);
        if (index != last) {
            tasks[index] = std::move(tasks[last]);
            m_index[tasks[index].id] = TaskLoc{deferred, index};
        }
        tasks.pop_back();
    }

    void TaskScheduler::compact(std::vector<Task>& tasks, bool deferred) {
        for (std::size_t i = 0; i < tasks.size();) {
            if (!tasks[i].cancelled) {
                ++i;
                continue;
            }
            tasks[i].callback.reset();
            eraseAt(tasks, deferred, i);
        }
    }

    void TaskScheduler::advance(double dt, lua_State* L) {
        (void)L;
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return;

        if (!m_deferred.empty()) {
            fireDeferred();
            compact(m_deferred, true);
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
            compact(m_timed, false);
        }
    }

    void TaskScheduler::clear() {
        for (auto& task : m_timed) {
            task.callback.reset();
        }
        for (auto& task : m_deferred) {
            task.callback.reset();
        }
        m_timed.clear();
        m_deferred.clear();
        m_index.clear();
    }

    bool TaskScheduler::full() const {
        return m_timed.size() + m_deferred.size() >= kMaxScheduledTasks;
    }

    std::size_t TaskScheduler::activeCount() const {
        std::size_t n = 0;
        for (auto const& task : m_timed) {
            if (!task.cancelled) ++n;
        }
        for (auto const& task : m_deferred) {
            if (!task.cancelled) ++n;
        }
        return n;
    }

#if defined(LUAUAPI_HOST_TESTS)
    bool TaskScheduler::isScheduled(std::uint64_t id) const {
        auto it = m_index.find(id);
        if (it == m_index.end()) {
            return false;
        }
        auto const& loc = it->second;
        auto const& tasks = loc.deferred ? m_deferred : m_timed;
        if (loc.index >= tasks.size()) {
            return false;
        }
        Task const& task = tasks[loc.index];
        return task.id == id && !task.cancelled;
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
    }

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
}
