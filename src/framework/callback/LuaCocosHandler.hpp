#pragma once

#include "framework/callback/LuaCallback.hpp"

#include <cocos2d.h>
#include <lua.h>
#include <memory>

namespace luax {
    namespace detail {
        bool invokeCocosCallback(
            std::shared_ptr<LuaCallback>& callback, int nargs, std::string_view context,
            LuaCallback::PushArgsFn pushArgs, void* pushCtx
        );
    }

    class LuaCocosHandlerBase : public cocos2d::CCObject {
    protected:
        std::shared_ptr<LuaCallback> m_callback;

        template <class Handler>
        static Handler* create(lua_State* L, int fnIndex) {
            luaL_checktype(L, fnIndex, LUA_TFUNCTION);
            auto* handler = new Handler();
            handler->m_callback = std::make_shared<LuaCallback>(L, fnIndex);
            handler->autorelease();
            return handler;
        }
    };

    class LuaMenuHandler final : public LuaCocosHandlerBase {
    public:
        static LuaMenuHandler* create(lua_State* L, int fnIndex);
        void onCallback(cocos2d::CCObject* sender);
    };

    class LuaScheduleHandler final : public LuaCocosHandlerBase {
    public:
        static LuaScheduleHandler* create(lua_State* L, int fnIndex);
        void onSchedule(float dt);
    };

    class LuaCallFuncHandler final : public LuaCocosHandlerBase {
    public:
        static LuaCallFuncHandler* create(lua_State* L, int fnIndex);
        void onCallFunc();
    };

    class LuaCallFuncNHandler final : public LuaCocosHandlerBase {
    public:
        static LuaCallFuncNHandler* create(lua_State* L, int fnIndex);
        void onCallFuncN(cocos2d::CCNode* node);
    };

    class LuaCallFuncNDHandler final : public LuaCocosHandlerBase {
    public:
        static LuaCallFuncNDHandler* create(lua_State* L, int fnIndex);
        void onCallFuncND(cocos2d::CCNode* node, void* data);
    };

    class LuaCallFuncOHandler final : public LuaCocosHandlerBase {
    public:
        static LuaCallFuncOHandler* create(lua_State* L, int fnIndex);
        void onCallFuncO(cocos2d::CCObject* obj);
    };
} // namespace luax
