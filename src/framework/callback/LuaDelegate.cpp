#include "framework/callback/LuaDelegate.hpp"

#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/callback/LuaTrampolineRegistry.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/usertype/Usertype.hpp"

#include <atomic>
#include <lualib.h>
#include <thread>

namespace luax {
    namespace {
        std::unordered_map<void*, std::weak_ptr<LuaRef>>& delegateTables() {
            static std::unordered_map<void*, std::weak_ptr<LuaRef>> tables;
            return tables;
        }

        void pruneExpiredDelegateTables() {
            auto& tables = delegateTables();
            for (auto it = tables.begin(); it != tables.end();) {
                auto table = it->second.lock();
                if (!table || !table->valid()) {
                    it = tables.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    } // namespace

    bool LuaDelegateBase::invokeTableField(
        std::shared_ptr<LuaRef> const& table, char const* field, char const* context, int nargs,
        int nresults, LuaCallback::PushArgsFn push, void* pushCtx, LuaCallback::PopResultsFn pop,
        void* popCtx
    ) {
        if (!table || !table->valid()) return false;
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime || !runtime->ready()) return false;
#if defined(GEODE_IS_MACOS)
        if (!Runtime::isMainThread()) {
            // #region agent log
            Runtime::debugThreadProbe(
                "post-fix",
                "H11,H12,H13",
                context,
                "adopting LuaDelegate thread before runtime state"
            );
            // #endregion
            Runtime::setMainThreadId(std::this_thread::get_id());
        }
#endif
        if (!Runtime::isMainThread()) {
            static std::atomic_bool s_loggedOffThreadDelegate{false};
            bool expected = false;
            if (s_loggedOffThreadDelegate.compare_exchange_strong(expected, true)) {
                // #region agent log
                Runtime::debugThreadProbe(
                    "next", "H6,H9", context, "first off-thread delegate invoke before runtime state"
                );
                // #endregion
            }
        }
        auto* L = runtime->state();
        if (!L) return false;

        int top = lua_gettop(L);
        if (!table->push()) return false;
        lua_getfield(L, -1, field);
        if (!lua_isfunction(L, -1)) {
            lua_settop(L, top);
            return false;
        }
        lua_remove(L, -2);

        LuaCallback cb;
        cb.reset(L, -1);
        lua_pop(L, 1);
        bool ok =
            cb.invoke(nargs, nresults, context, kHookScriptDeadlineMs, push, pushCtx, pop, popCtx);
        if (!ok) {
            logCallbackFailure(context);
        }
        return ok;
    }

    void LuaDelegateBase::checkDelegateTable(lua_State* L, int idx) {
        luaL_checktype(L, idx, LUA_TTABLE);
    }

    std::shared_ptr<LuaRef> LuaDelegateBase::captureTable(lua_State* L, int idx) {
        return std::make_shared<LuaRef>(L, idx);
    }

    void LuaDelegateBase::invokeTableVoid(
        std::shared_ptr<LuaRef> const& table, char const* field, char const* context, int nargs,
        LuaCallback::PushArgsFn push, void* pushCtx
    ) {
        invokeTableField(table, field, context, nargs, 0, push, pushCtx, nullptr, nullptr);
    }

    void LuaDelegateBase::registerInterface(void* ptr, std::shared_ptr<LuaRef> const& table) {
        if (!ptr || !table) return;
        pruneExpiredDelegateTables();
        delegateTables()[ptr] = table;
    }

    void LuaDelegateBase::unregisterInterface(void* ptr) {
        delegateTables().erase(ptr);
    }

    bool tryPushBoundDelegateTable(lua_State* L, void* delegatePtr) {
        if (!delegatePtr) {
            lua_pushnil(L);
            return true;
        }
        pruneExpiredDelegateTables();
        auto it = delegateTables().find(delegatePtr);
        if (it == delegateTables().end()) {
            lua_pushnil(L);
            return true;
        }
        auto table = it->second.lock();
        if (!table || !table->valid()) {
            delegateTables().erase(it);
            lua_pushnil(L);
            return true;
        }
        return table->push();
    }

    void anchorDelegate(cocos2d::CCObject* anchor, cocos2d::CCObject* trampoline) {
        anchorTrampoline(anchor, trampoline);
    }
} // namespace luax
