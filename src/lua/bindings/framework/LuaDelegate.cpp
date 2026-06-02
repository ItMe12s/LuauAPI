#include "LuaDelegate.hpp"

#include "Stack.hpp"
#include "Usertype.hpp"
#include "lua/Config.hpp"
#include "lua/bindings/framework/LuaTrampolineRegistry.hpp"
#include "lua/runtime/Runtime.hpp"

#include <lualib.h>

namespace luax {
    namespace {
        std::unordered_map<void*, std::weak_ptr<LuaRef>>& delegateTables() {
            static auto* value = new std::unordered_map<void*, std::weak_ptr<LuaRef>>();
            return *value;
        }
    }

    bool LuaDelegateBase::invokeTableField(
            std::shared_ptr<LuaRef> const& table,
            char const* field,
            char const* context,
            int nargs,
            int nresults,
            LuaCallback::PushArgsFn push,
            void* pushCtx,
            LuaCallback::PopResultsFn pop,
            void* popCtx
        ) {
            if (!table || !table->valid()) return false;
            auto* runtime = Runtime::getIfInitialized();
            if (!runtime || !runtime->ready()) return false;
            auto* L = runtime->state();
            if (!L || !table->push()) return false;

            int top = lua_gettop(L);
            lua_getfield(L, -1, field);
            if (!lua_isfunction(L, -1)) {
                lua_settop(L, top);
                return false;
            }
            lua_remove(L, -2);

            LuaCallback cb;
            cb.reset(L, -1);
            lua_pop(L, 1);
            return cb.invoke(nargs, nresults, context, kHookScriptDeadlineMs, push, pushCtx, pop, popCtx);
    }

    void LuaDelegateBase::checkDelegateTable(lua_State* L, int idx) {
        luaL_checktype(L, idx, LUA_TTABLE);
    }

    std::shared_ptr<LuaRef> LuaDelegateBase::captureTable(lua_State* L, int idx) {
        return std::make_shared<LuaRef>(L, idx);
    }

    void LuaDelegateBase::invokeTableVoid(
        std::shared_ptr<LuaRef> const& table,
        char const* field,
        char const* context,
        int nargs,
        LuaCallback::PushArgsFn push,
        void* pushCtx
    ) {
        invokeTableField(table, field, context, nargs, 0, push, pushCtx, nullptr, nullptr);
    }

    bool LuaDelegateBase::invokeTableBool(
        std::shared_ptr<LuaRef> const& table,
        char const* field,
        bool defaultValue,
        char const* context,
        int nargs,
        LuaCallback::PushArgsFn push,
        void* pushCtx
    ) {
        bool result = defaultValue;
        if (!invokeTableField(
                table,
                field,
                context,
                nargs,
                1,
                push,
                pushCtx,
                +[](lua_State* L, void* raw) {
                    *static_cast<bool*>(raw) = luax::check<bool>(L, -1, "delegate callback return");
                },
                &result
            )) {
            return defaultValue;
        }
        return result;
    }

    int LuaDelegateBase::invokeTableInt(
        std::shared_ptr<LuaRef> const& table,
        char const* field,
        int defaultValue,
        char const* context,
        int nargs,
        LuaCallback::PushArgsFn push,
        void* pushCtx
    ) {
        int result = defaultValue;
        if (!invokeTableField(
                table,
                field,
                context,
                nargs,
                1,
                push,
                pushCtx,
                +[](lua_State* L, void* raw) {
                    *static_cast<int*>(raw) = luax::check<int>(L, -1, "delegate callback return");
                },
                &result
            )) {
            return defaultValue;
        }
        return result;
    }

    std::string LuaDelegateBase::invokeTableString(
        std::shared_ptr<LuaRef> const& table,
        char const* field,
        std::string defaultValue,
        char const* context,
        int nargs,
        LuaCallback::PushArgsFn push,
        void* pushCtx
    ) {
        std::string result = defaultValue;
        if (!invokeTableField(
                table,
                field,
                context,
                nargs,
                1,
                push,
                pushCtx,
                +[](lua_State* L, void* raw) {
                    *static_cast<std::string*>(raw) =
                        luax::check<std::string>(L, -1, "delegate callback return");
                },
                &result
            )) {
            return defaultValue;
        }
        return result;
    }

    void LuaDelegateBase::registerInterface(void* ptr, std::shared_ptr<LuaRef> const& table) {
        if (!ptr || !table) return;
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
        auto it = delegateTables().find(delegatePtr);
        if (it == delegateTables().end()) {
            lua_pushnil(L);
            return true;
        }
        auto table = it->second.lock();
        if (!table || !table->valid()) {
            lua_pushnil(L);
            return true;
        }
        return table->push();
    }

    void anchorDelegate(cocos2d::CCObject* anchor, cocos2d::CCObject* trampoline) {
        anchorTrampoline(anchor, trampoline);
    }
}
