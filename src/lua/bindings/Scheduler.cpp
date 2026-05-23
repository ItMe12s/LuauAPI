#include "../Binding.hpp"
#include "../Runtime.hpp"
#include "internal/LuaRef.hpp"
#include "internal/Ref.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <lualib.h>

#include <cmath>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
    using namespace luax;

    constexpr float kMinInterval        = 1e-4f;
    constexpr float kMaxInterval        = 3600.f;
    constexpr float kMaxStep            = 0.25f;
    constexpr int   kFailureBudget      = 5;
    constexpr int   kCallbackDeadlineMs = 50;

    class LuaScheduler : public cocos2d::CCObject {
    public:
        struct Entry {
            uint64_t id = 0;
            float interval = 0.f;
            float accumulator = 0.f;
            int consecutiveFailures = 0;
            bool dead = false;
            LuaRef callback;
        };

        uint64_t add(LuaRef cb, float interval) {
            auto id = m_nextId++;
            auto entry = std::make_unique<Entry>();
            entry->id = id;
            entry->interval = interval;
            entry->callback = std::move(cb);
            m_entries.emplace(id, std::move(entry));
            return id;
        }

        bool remove(uint64_t id) {
            auto it = m_entries.find(id);
            if (it == m_entries.end()) return false;
            if (m_iterating) {
                it->second->dead = true;
            } else {
                m_entries.erase(it);
            }
            return true;
        }

        void tick(float dt) {
            if (m_iterating) return;
            if (dt > kMaxStep) dt = kMaxStep;
            if (dt <= 0.f) return;

            m_iterating = true;

            std::vector<uint64_t> ids;
            ids.reserve(m_entries.size());
            for (auto const& kv : m_entries) ids.push_back(kv.first);

            for (auto id : ids) {
                auto it = m_entries.find(id);
                if (it == m_entries.end()) continue;
                Entry& e = *it->second;
                if (e.dead) continue;

                e.accumulator += dt;
                while (!e.dead && e.accumulator >= e.interval) {
                    e.accumulator -= e.interval;
                    if (invoke(e)) {
                        e.consecutiveFailures = 0;
                    } else if (++e.consecutiveFailures >= kFailureBudget) {
                        geode::log::warn(
                            "[lua:scheduler] entry {} disabled after {} consecutive failures",
                            e.id, e.consecutiveFailures);
                        e.dead = true;
                    }
                }
            }

            m_iterating = false;

            for (auto it = m_entries.begin(); it != m_entries.end();) {
                if (it->second->dead) it = m_entries.erase(it);
                else ++it;
            }
        }

        void clear() { m_entries.clear(); }

    private:
        bool invoke(Entry& e) {
            auto& runtime = Runtime::instance();
            if (!runtime.ready()) return false;
            auto* L = runtime.state();
            if (!e.callback.push()) return false;
            if (!lua_isfunction(L, -1)) {
                lua_pop(L, 1);
                return false;
            }
            return runtime.protectedCall(0, 0, "scheduler callback", kCallbackDeadlineMs);
        }

        std::unordered_map<uint64_t, std::unique_ptr<Entry>> m_entries;
        uint64_t m_nextId = 1;
        bool m_iterating = false;
    };

    LuaScheduler* g_scheduler = nullptr;

    void shutdownScheduler() {
        if (!g_scheduler) return;
        if (auto* director = cocos2d::CCDirector::sharedDirector()) {
            if (auto* sched = director->getScheduler()) {
                sched->unscheduleSelector(schedule_selector(LuaScheduler::tick), g_scheduler);
            }
        }
        g_scheduler->clear();
        g_scheduler->release();
        g_scheduler = nullptr;
    }

    LuaScheduler& ensureScheduler() {
        if (!g_scheduler) {
            g_scheduler = new LuaScheduler();
            g_scheduler->retain();
            cocos2d::CCDirector::sharedDirector()->getScheduler()->scheduleSelector(
                schedule_selector(LuaScheduler::tick),
                g_scheduler, 0.f, kCCRepeatForever, 0.f, false);
            Runtime::instance().registerShutdownHook(&shutdownScheduler);
        }
        return *g_scheduler;
    }

    int scheduler_schedule(lua_State* L) {
        assertMainThread();
        if (!lua_isfunction(L, 1)) {
            luaL_error(L, "cocos2d.scheduler.schedule expected function at arg 1");
        }
        if (!lua_isnumber(L, 2)) {
            luaL_error(L, "cocos2d.scheduler.schedule expected number at arg 2");
        }
        double n = lua_tonumber(L, 2);
        if (!std::isfinite(n) || n < static_cast<double>(kMinInterval) || n > static_cast<double>(kMaxInterval)) {
            luaL_error(L, "cocos2d.scheduler.schedule interval out of range (%g..%g)",
                static_cast<double>(kMinInterval), static_cast<double>(kMaxInterval));
        }

        LuaRef cb(L, 1);
        auto id = ensureScheduler().add(std::move(cb), static_cast<float>(n));
        lua_pushnumber(L, static_cast<double>(id));
        return 1;
    }

    int scheduler_unschedule(lua_State* L) {
        assertMainThread();
        if (!lua_isnumber(L, 1)) {
            luaL_error(L, "cocos2d.scheduler.unschedule expected number at arg 1");
        }
        double n = lua_tonumber(L, 1);
        bool removed = false;
        if (g_scheduler && std::isfinite(n) && n >= 1.0) {
            removed = g_scheduler->remove(static_cast<uint64_t>(n));
        }
        lua_pushboolean(L, removed);
        return 1;
    }

    void bindScheduler(lua_State* L) {
        getOrCreateTable(L, "geode.cocos2d.scheduler");
        setTableCFunction(L, -1, "schedule",   &scheduler_schedule);
        setTableCFunction(L, -1, "unschedule", &scheduler_unschedule);
        lua_pop(L, 1);
    }

    LUAX_BINDING_PRIORITY(Scheduler, bindScheduler, 30)
}
