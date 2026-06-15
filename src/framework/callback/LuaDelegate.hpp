#pragma once

#include "core/Config.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/usertype/LuaRef.hpp"
#include "framework/usertype/Usertype.hpp"

#include <cocos2d.h>
#include <lua.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace luax {
    class LuaDelegateBase {
    public:
        static void checkDelegateTable(lua_State* L, int idx);
        static std::shared_ptr<LuaRef> captureTable(lua_State* L, int idx);

        static void invokeTableVoid(
            std::shared_ptr<LuaRef> const& table, char const* field, char const* context, int nargs,
            LuaCallback::PushArgsFn push = nullptr, void* pushCtx = nullptr
        );

        template <class T>
        static T invokeTableValue(
            std::shared_ptr<LuaRef> const& table, char const* field, T defaultValue,
            char const* context, int nargs, LuaCallback::PushArgsFn push = nullptr,
            void* pushCtx = nullptr
        ) {
            T result = defaultValue;
            if (!invokeTableField(
                    table,
                    field,
                    context,
                    nargs,
                    1,
                    push,
                    pushCtx,
                    +[](lua_State* L, void* raw) {
                        *static_cast<T*>(raw) = luax::check<T>(L, -1, "delegate callback return");
                    },
                    &result
                )) {
                return defaultValue;
            }
            return result;
        }

        template <class T>
        static T* invokeTableObject(
            std::shared_ptr<LuaRef> const& table, char const* field, T* defaultValue,
            char const* context, int nargs, LuaCallback::PushArgsFn push = nullptr,
            void* pushCtx = nullptr
        ) {
            T* result = defaultValue;
            if (!invokeTableField(
                    table,
                    field,
                    context,
                    nargs,
                    1,
                    push,
                    pushCtx,
                    +[](lua_State* L, void* raw) {
                        auto* slot = static_cast<T**>(raw);
                        *slot = Usertype<T>::check(L, -1, "delegate callback return");
                    },
                    &result
                )) {
                return defaultValue;
            }
            return result;
        }

        static void registerInterface(void* ptr, std::shared_ptr<LuaRef> const& table);
        static void unregisterInterface(void* ptr);

    private:
        static bool invokeTableField(
            std::shared_ptr<LuaRef> const& table, char const* field, char const* context, int nargs,
            int nresults, LuaCallback::PushArgsFn push, void* pushCtx,
            LuaCallback::PopResultsFn pop, void* popCtx
        );
    };

    bool tryPushBoundDelegateTable(lua_State* L, void* delegatePtr);
    void anchorDelegate(cocos2d::CCObject* anchor, cocos2d::CCObject* trampoline);
} // namespace luax
