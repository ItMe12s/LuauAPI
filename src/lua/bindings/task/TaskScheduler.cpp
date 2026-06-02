#include "TaskScheduler.hpp"

#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

#include <algorithm>

#include <Geode/Geode.hpp>

namespace luax {
    TaskScheduler& TaskScheduler::get() {
        static auto* value = new TaskScheduler();
        return *value;
    }

    std::uint64_t TaskScheduler::add(LuaRef callback, double delaySeconds, double intervalSeconds) {
        if (m_tasks.size() >= kMaxScheduledTasks) {
            return 0;
        }
        Task task;
        task.id = m_nextId++;
        task.callback = std::move(callback);
        task.remaining = delaySeconds;
        task.interval = intervalSeconds;
        m_tasks.push_back(std::move(task));
        return m_tasks.back().id;
    }

    void TaskScheduler::cancel(std::uint64_t id) {
        if (auto* task = find(id)) {
            task->cancelled = true;
        }
    }

    TaskScheduler::Task* TaskScheduler::find(std::uint64_t id) {
        for (auto& task : m_tasks) {
            if (task.id == id && !task.cancelled) {
                return &task;
            }
        }
        return nullptr;
    }

    bool TaskScheduler::fire(Task& task) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return false;
        if (!task.callback.push()) return false;
        auto* L = task.callback.luaState();
        if (!L) return false;
        int top = lua_gettop(L);
        Runtime::ResourcesRootScope scope(*runtime, task.callback.resourcesRoot());
        bool ok = runtime->protectedCall(0, 0, "task", kHookScriptDeadlineMs);
        lua_settop(L, top);
        return ok;
    }

    void TaskScheduler::advance(double dt, lua_State* L) {
        (void)L;
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || m_tasks.empty()) return;

        std::vector<std::uint64_t> due;
        std::size_t count = m_tasks.size();
        for (std::size_t i = 0; i < count; ++i) {
            Task& task = m_tasks[i];
            if (task.cancelled) continue;
            if (task.interval < 0.0) {
                due.push_back(task.id);
            } else {
                task.remaining -= dt;
                if (task.remaining <= 0.0) {
                    due.push_back(task.id);
                }
            }
        }

        for (std::uint64_t id : due) {
            Task* task = find(id);
            if (!task) continue;

            bool ok = fire(*task);

            task = find(id);
            if (!task) continue;

            if (!ok) {
                task->cancelled = true;
            } else if (task->interval > 0.0) {
                task->remaining = task->interval;
            } else {
                task->cancelled = true;
            }
        }

        for (auto& task : m_tasks) {
            if (task.cancelled) task.callback.reset();
        }
        m_tasks.erase(
            std::remove_if(m_tasks.begin(), m_tasks.end(), [](Task const& t) { return t.cancelled; }),
            m_tasks.end()
        );
    }

    void TaskScheduler::clear() {
        for (auto& task : m_tasks) {
            task.callback.reset();
        }
        m_tasks.clear();
    }

    bool TaskScheduler::full() const {
        return m_tasks.size() >= kMaxScheduledTasks;
    }

    std::size_t TaskScheduler::activeCount() const {
        std::size_t n = 0;
        for (auto const& task : m_tasks) {
            if (!task.cancelled) ++n;
        }
        return n;
    }

#if !defined(LUAUAPI_HOST_TESTS)
    namespace {
        class TaskTickNode : public cocos2d::CCNode {
        public:
            void update(float dt) override {
                auto* runtime = Runtime::getIfInitialized();
                if (!runtime) return;
                auto* L = runtime->state();
                if (!L) return;
                TaskScheduler::get().advance(static_cast<double>(dt), L);
            }
        };

        TaskTickNode* g_tickNode = nullptr;
    }

    void armTaskTick() {
        if (g_tickNode) return;
        auto* director = cocos2d::CCDirector::sharedDirector();
        if (!director) return;
        auto* scheduler = director->getScheduler();
        if (!scheduler) return;
        g_tickNode = new TaskTickNode();
        g_tickNode->init();
        scheduler->scheduleUpdateForTarget(g_tickNode, 0, false);
    }

    void disarmTaskTick() {
        if (!g_tickNode) return;
        if (auto* director = cocos2d::CCDirector::sharedDirector()) {
            if (auto* scheduler = director->getScheduler()) {
                scheduler->unscheduleUpdateForTarget(g_tickNode);
            }
        }
        g_tickNode->release();
        g_tickNode = nullptr;
    }
#else
    void armTaskTick() {}

    void disarmTaskTick() {}
#endif
}
