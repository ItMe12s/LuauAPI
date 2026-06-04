#pragma once

#include "LuaRef.hpp"

#include <cocos2d.h>
#include <lua.h>

namespace luax {
    class Fields {
    public:
        static void push(lua_State* L, cocos2d::CCNode* node);
        static bool tryPush(lua_State* L, cocos2d::CCNode* node);
        static void evict(cocos2d::CCNode* node);
        static void evict(cocos2d::CCObject* object);
        static void evictIfFinalRelease(cocos2d::CCObject* object);
        static void clear();
    };
} // namespace luax
