#include "bindings/task/TaskScheduler.hpp"

#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/schedule/ScheduledCallback.hpp"
#include "framework/usertype/DeferredRelease.hpp"

#include <Geode/Geode.hpp>
#include <thread>

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
        task.callback = std::move(callback);
        m_deferred.insertWithId(id, std::move(task));
        m_deferredIds[id] = true;
        return id;
    }

    void TaskScheduler::cancel(std::uint64_t id) {
        m_timed.cancel(id);
        m_deferred.cancel(id);
    }

    bool TaskScheduler::fire(Task& task) {
        return fireProtectedCallback(task.callback, "task", kHookScriptDeadlineMs);
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

    void TaskScheduler::eraseTaskAt(ScheduledSlotStore<Task>& store, std::size_t index) {
        std::uint64_t const id = store.slots().idAt(index);
        store.slots().eraseAt(index);
        m_deferredIds.erase(id);
    }

    void TaskScheduler::compact(ScheduledSlotStore<Task>& store) {
        for (std::size_t i = 0; i < store.size();) {
            if (!store[i].cancelled) {
                ++i;
                continue;
            }
            store[i].callback.reset();
            eraseTaskAt(store, i);
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
        m_timed.clear();
        m_deferred.clear();
        m_deferredIds.clear();
    }

    bool TaskScheduler::full() const {
        return activeCount() >= kMaxScheduledTasks;
    }

    std::size_t TaskScheduler::activeCount() const {
        return m_timed.activeCount() + m_deferred.activeCount();
    }

#if defined(LUAUAPI_HOST_TESTS)
    bool TaskScheduler::isScheduled(std::uint64_t id) const {
        auto it = m_deferredIds.find(id);
        if (it == m_deferredIds.end()) {
            return false;
        }
        auto const& store = it->second ? m_deferred : m_timed;
        return store.isScheduled(id);
    }
#endif

#if !defined(LUAUAPI_HOST_TESTS)
    namespace {
        class TaskTickNode final : public cocos2d::CCNode {
        public:
            void update(float dt) override {
    #if defined(GEODE_IS_MACOS)
                if (!Runtime::isMainThread()) {
                    Runtime::setMainThreadId(std::this_thread::get_id());
                }
    #endif
                auto* runtime = Runtime::getIfInitialized();
                if (!runtime) return;
                auto* L = runtime->state();
                if (!L) return;
                drainDeferredReleases();
                TaskScheduler::get().advance(static_cast<double>(dt), L);
            }
        };

        TaskTickNode* s_tickNode = nullptr;
        bool s_armPending = false;
        bool s_retryQueued = false;
        bool s_directorErrorLogged = false;
        bool s_schedulerErrorLogged = false;

        bool tryArmTaskTick() {
            if (s_tickNode) return true;
            auto* director = cocos2d::CCDirector::sharedDirector();
            if (!director) {
                if (!s_directorErrorLogged) {
                    s_directorErrorLogged = true;
                    geode::log::error("task scheduler: CCDirector unavailable, task tick not armed");
                }
                return false;
            }
            auto* scheduler = director->getScheduler();
            if (!scheduler) {
                if (!s_schedulerErrorLogged) {
                    s_schedulerErrorLogged = true;
                    geode::log::error(
                        "task scheduler: cocos2d scheduler unavailable, task tick not armed"
                    );
                }
                return false;
            }
            s_tickNode = new TaskTickNode();
            s_tickNode->init();
            scheduler->scheduleUpdateForTarget(s_tickNode, 0, false);
            s_armPending = false;
            return true;
        }

        void scheduleArmRetry() {
            if (s_retryQueued || s_tickNode) return;
            s_armPending = true;
            s_retryQueued = true;
            geode::queueInMainThread([] {
                s_retryQueued = false;
                if (!s_armPending || s_tickNode) return;
                if (tryArmTaskTick()) return;
                scheduleArmRetry();
            });
        }
    } // namespace

    bool armTaskTick() {
        if (tryArmTaskTick()) return true;
        scheduleArmRetry();
        return false;
    }

    void disarmTaskTick() {
        s_armPending = false;
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
    bool armTaskTick() {
        return false;
    }

    void disarmTaskTick() {}
#endif
} // namespace luax
