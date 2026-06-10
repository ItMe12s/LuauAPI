#pragma once

#include "framework/callback/LuaCallback.hpp"

#include <cocos2d.h>
#include <memory>

namespace luax {
    class LuaScheduleHandler final : public cocos2d::CCObject {
    public:
        static LuaScheduleHandler* create(lua_State* L, int fnIndex);
        void onSchedule(float dt);

    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    class LuaCallFuncHandler final : public cocos2d::CCObject {
    public:
        static LuaCallFuncHandler* create(lua_State* L, int fnIndex);
        void onCallFunc();

    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    class LuaCallFuncNHandler final : public cocos2d::CCObject {
    public:
        static LuaCallFuncNHandler* create(lua_State* L, int fnIndex);
        void onCallFuncN(cocos2d::CCNode* node);

    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    class LuaCallFuncNDHandler final : public cocos2d::CCObject {
    public:
        static LuaCallFuncNDHandler* create(lua_State* L, int fnIndex);
        void onCallFuncND(cocos2d::CCNode* node, void* data);

    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    class LuaCallFuncOHandler final : public cocos2d::CCObject {
    public:
        static LuaCallFuncOHandler* create(lua_State* L, int fnIndex);
        void onCallFuncO(cocos2d::CCObject* obj);

    private:
        std::shared_ptr<LuaCallback> m_callback;
    };
} // namespace luax
