#pragma once

#include "lua/bindings/framework/LuaCallback.hpp"

#include <cocos2d.h>

#include <memory>

namespace luax {
    class LuaScheduleHandler : public cocos2d::CCObject {
    public:
        static LuaScheduleHandler* create(lua_State* L, int fnIndex);
        void onSchedule(float dt);
    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    class LuaCallFuncHandler : public cocos2d::CCObject {
    public:
        static LuaCallFuncHandler* create(lua_State* L, int fnIndex);
        void onCallFunc();
    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    class LuaCallFuncNHandler : public cocos2d::CCObject {
    public:
        static LuaCallFuncNHandler* create(lua_State* L, int fnIndex);
        void onCallFuncN(cocos2d::CCNode* node);
    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    class LuaCallFuncNDHandler : public cocos2d::CCObject {
    public:
        static LuaCallFuncNDHandler* create(lua_State* L, int fnIndex);
        void onCallFuncND(cocos2d::CCNode* node, void* data);
    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    class LuaCallFuncOHandler : public cocos2d::CCObject {
    public:
        static LuaCallFuncOHandler* create(lua_State* L, int fnIndex);
        void onCallFuncO(cocos2d::CCObject* obj);
    private:
        std::shared_ptr<LuaCallback> m_callback;
    };
}
