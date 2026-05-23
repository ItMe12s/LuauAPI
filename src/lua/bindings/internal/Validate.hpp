#pragma once

#include "Usertype.hpp"

#include <cocos2d.h>
#include <lua.h>
#include <stdexcept>
#include <string>

namespace luax {
    template <class T>
    inline T* requirePtr(T* value, char const* method, char const* typeName) {
        if (!value) {
            throw std::runtime_error(std::string(method) + " expected " + typeName);
        }
        return value;
    }

    inline cocos2d::CCObject* requireObject(cocos2d::CCObject* o, char const* m) { return requirePtr(o, m, "CCObject"); }
    inline cocos2d::CCNode*   requireNode  (cocos2d::CCNode* n, char const* m)   { return requirePtr(n, m, "CCNode"); }
    inline cocos2d::CCSprite* requireSprite(cocos2d::CCSprite* s, char const* m) { return requirePtr(s, m, "CCSprite"); }
    inline cocos2d::CCLayer*  requireLayer (cocos2d::CCLayer* l, char const* m)  { return requirePtr(l, m, "CCLayer"); }
    inline cocos2d::CCAction* requireAction(cocos2d::CCAction* a, char const* m) { return requirePtr(a, m, "CCAction"); }
    inline cocos2d::CCFiniteTimeAction* requireFiniteTimeAction(cocos2d::CCFiniteTimeAction* a, char const* m) {
        return requirePtr(a, m, "CCFiniteTimeAction");
    }
    inline cocos2d::CCActionInterval* requireActionInterval(cocos2d::CCActionInterval* a, char const* m) {
        return requirePtr(a, m, "CCActionInterval");
    }
    inline cocos2d::CCSpriteFrame* requireSpriteFrame(cocos2d::CCSpriteFrame* f, char const* m) {
        return requirePtr(f, m, "CCSpriteFrame");
    }
}
