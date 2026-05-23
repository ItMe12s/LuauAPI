#include "../Binding.hpp"
#include "internal/Ref.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>

namespace {
    using namespace luax;

    int ccobject_getTag(lua_State* L) {
        auto self = Usertype<cocos2d::CCObject>::check(L, 1, "CCObject:getTag");
        push(L, self->getTag());
        return 1;
    }

    int ccobject_setTag(lua_State* L) {
        auto self = Usertype<cocos2d::CCObject>::check(L, 1, "CCObject:setTag");
        assertMainThread();
        self->setTag(check<int>(L, 2, "CCObject:setTag"));
        return 0;
    }

    int ccobject_retain(lua_State* L) {
        auto self = Usertype<cocos2d::CCObject>::check(L, 1, "CCObject:retain");
        retainLuaRef(self, "CCObject:retain");
        return 0;
    }

    int ccobject_release(lua_State* L) {
        auto self = Usertype<cocos2d::CCObject>::check(L, 1, "CCObject:release");
        releaseLuaRef(self, "CCObject:release");
        return 0;
    }

    int ccobject_retainCount(lua_State* L) {
        auto self = Usertype<cocos2d::CCObject>::check(L, 1, "CCObject:retainCount");
        push(L, self->retainCount());
        return 1;
    }

    void bindCCObject(lua_State* L) {
        Usertype<cocos2d::CCObject>::registerType(L, "CCObject");
        Usertype<cocos2d::CCObject>::method(L, "getTag",      &ccobject_getTag);
        Usertype<cocos2d::CCObject>::method(L, "setTag",      &ccobject_setTag);
        Usertype<cocos2d::CCObject>::method(L, "retain",      &ccobject_retain);
        Usertype<cocos2d::CCObject>::method(L, "release",     &ccobject_release);
        Usertype<cocos2d::CCObject>::method(L, "retainCount", &ccobject_retainCount);
    }

    LUAX_BINDING_PRIORITY(CCObject, bindCCObject, 0)
}
