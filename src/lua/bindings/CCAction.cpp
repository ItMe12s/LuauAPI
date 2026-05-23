#include "../Binding.hpp"
#include "internal/Ref.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Usertype.hpp"
#include "internal/Validate.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <stdexcept>
#include <string>

namespace {
    using namespace luax;

    cocos2d::CCArray* actionsFromTable(lua_State* L, int idx, char const* method) {
        if (lua_type(L, idx) != LUA_TTABLE) {
            luaL_error(L, "%s expected table of actions", method);
        }
        int count = static_cast<int>(lua_objlen(L, idx));
        if (count == 0) {
            luaL_error(L, "%s expected at least one action", method);
        }
        auto array = cocos2d::CCArray::createWithCapacity(static_cast<unsigned int>(count));
        for (int i = 1; i <= count; ++i) {
            lua_rawgeti(L, idx, i);
            auto action = Usertype<cocos2d::CCFiniteTimeAction>::check(L, -1, method);
            array->addObject(requireFiniteTimeAction(action, method));
            lua_pop(L, 1);
        }
        return array;
    }

    int ccaction_getTag(lua_State* L) {
        push(L, Usertype<cocos2d::CCAction>::check(L, 1, "CCAction:getTag")->getTag());
        return 1;
    }
    int ccaction_setTag(lua_State* L) {
        auto self = Usertype<cocos2d::CCAction>::check(L, 1, "CCAction:setTag");
        assertMainThread();
        self->setTag(check<int>(L, 2, "CCAction:setTag"));
        return 0;
    }
    int ccaction_isDone(lua_State* L) {
        push(L, Usertype<cocos2d::CCAction>::check(L, 1, "CCAction:isDone")->isDone());
        return 1;
    }
    int ccaction_retain(lua_State* L) {
        auto self = Usertype<cocos2d::CCAction>::check(L, 1, "CCAction:retain");
        retainLuaRef(self, "CCAction:retain");
        return 0;
    }
    int ccaction_release(lua_State* L) {
        auto self = Usertype<cocos2d::CCAction>::check(L, 1, "CCAction:release");
        releaseLuaRef(self, "CCAction:release");
        return 0;
    }
    int ccaction_retainCount(lua_State* L) {
        push(L, Usertype<cocos2d::CCAction>::check(L, 1, "CCAction:retainCount")->retainCount());
        return 1;
    }

    int ccfinitetimeaction_getDuration(lua_State* L) {
        push(L, Usertype<cocos2d::CCFiniteTimeAction>::check(L, 1, "CCFiniteTimeAction:getDuration")->getDuration());
        return 1;
    }
    int ccfinitetimeaction_setDuration(lua_State* L) {
        auto self = Usertype<cocos2d::CCFiniteTimeAction>::check(L, 1, "CCFiniteTimeAction:setDuration");
        assertMainThread();
        self->setDuration(check<float>(L, 2, "CCFiniteTimeAction:setDuration"));
        return 0;
    }

    int ccactioninterval_getElapsed(lua_State* L) {
        push(L, Usertype<cocos2d::CCActionInterval>::check(L, 1, "CCActionInterval:getElapsed")->getElapsed());
        return 1;
    }

    int ccmoveto_create(lua_State* L) {
        assertMainThread();
        float d = check<float>(L, 1, "CCMoveTo:create");
        float x = check<float>(L, 2, "CCMoveTo:create");
        float y = check<float>(L, 3, "CCMoveTo:create");
        Usertype<cocos2d::CCMoveTo>::pushOwned(L, cocos2d::CCMoveTo::create(d, { x, y }));
        return 1;
    }
    int ccmoveby_create(lua_State* L) {
        assertMainThread();
        float d = check<float>(L, 1, "CCMoveBy:create");
        float x = check<float>(L, 2, "CCMoveBy:create");
        float y = check<float>(L, 3, "CCMoveBy:create");
        Usertype<cocos2d::CCMoveBy>::pushOwned(L, cocos2d::CCMoveBy::create(d, { x, y }));
        return 1;
    }
    int ccscaleto_create(lua_State* L) {
        assertMainThread();
        float d = check<float>(L, 1, "CCScaleTo:create");
        if (lua_gettop(L) == 2) {
            Usertype<cocos2d::CCScaleTo>::pushOwned(L, cocos2d::CCScaleTo::create(d, check<float>(L, 2, "CCScaleTo:create")));
        } else {
            float sx = check<float>(L, 2, "CCScaleTo:create");
            float sy = check<float>(L, 3, "CCScaleTo:create");
            Usertype<cocos2d::CCScaleTo>::pushOwned(L, cocos2d::CCScaleTo::create(d, sx, sy));
        }
        return 1;
    }
    int ccfadein_create(lua_State* L) {
        assertMainThread();
        Usertype<cocos2d::CCFadeIn>::pushOwned(L, cocos2d::CCFadeIn::create(check<float>(L, 1, "CCFadeIn:create")));
        return 1;
    }
    int ccfadeout_create(lua_State* L) {
        assertMainThread();
        Usertype<cocos2d::CCFadeOut>::pushOwned(L, cocos2d::CCFadeOut::create(check<float>(L, 1, "CCFadeOut:create")));
        return 1;
    }
    int ccdelaytime_create(lua_State* L) {
        assertMainThread();
        Usertype<cocos2d::CCDelayTime>::pushOwned(L, cocos2d::CCDelayTime::create(check<float>(L, 1, "CCDelayTime:create")));
        return 1;
    }
    int ccsequence_create(lua_State* L) {
        assertMainThread();
        auto arr = actionsFromTable(L, 1, "CCSequence:create");
        Usertype<cocos2d::CCSequence>::pushOwned(L, cocos2d::CCSequence::create(arr));
        return 1;
    }
    int ccrepeatforever_create(lua_State* L) {
        assertMainThread();
        auto action = Usertype<cocos2d::CCActionInterval>::check(L, 1, "CCRepeatForever:create");
        Usertype<cocos2d::CCRepeatForever>::pushOwned(L,
            cocos2d::CCRepeatForever::create(requireActionInterval(action, "CCRepeatForever:create")));
        return 1;
    }

    template <class T>
    int ccease_create(lua_State* L) {
        assertMainThread();
        auto action = Usertype<cocos2d::CCActionInterval>::check(L, 1, "CCActionEase:create");
        Usertype<T>::pushOwned(L, T::create(requireActionInterval(action, "CCActionEase:create")));
        return 1;
    }

    template <class T>
    int cceaserate_create(lua_State* L) {
        assertMainThread();
        auto action = Usertype<cocos2d::CCActionInterval>::check(L, 1, "CCEaseRateAction:create");
        float rate = check<float>(L, 2, "CCEaseRateAction:create");
        Usertype<T>::pushOwned(L, T::create(requireActionInterval(action, "CCEaseRateAction:create"), rate));
        return 1;
    }

    template <class T>
    int cceaseelastic_create(lua_State* L) {
        assertMainThread();
        auto action = Usertype<cocos2d::CCActionInterval>::check(L, 1, "CCEaseElastic:create");
        auto checked = requireActionInterval(action, "CCEaseElastic:create");
        if (lua_gettop(L) >= 2) {
            Usertype<T>::pushOwned(L, T::create(checked, check<float>(L, 2, "CCEaseElastic:create")));
        } else {
            Usertype<T>::pushOwned(L, T::create(checked));
        }
        return 1;
    }

    int cceaserateaction_getRate(lua_State* L) {
        push(L, Usertype<cocos2d::CCEaseRateAction>::check(L, 1, "CCEaseRateAction:getRate")->getRate());
        return 1;
    }
    int cceaserateaction_setRate(lua_State* L) {
        auto self = Usertype<cocos2d::CCEaseRateAction>::check(L, 1, "CCEaseRateAction:setRate");
        assertMainThread();
        self->setRate(check<float>(L, 2, "CCEaseRateAction:setRate"));
        return 0;
    }
    int cceaseelastic_getPeriod(lua_State* L) {
        push(L, Usertype<cocos2d::CCEaseElastic>::check(L, 1, "CCEaseElastic:getPeriod")->getPeriod());
        return 1;
    }
    int cceaseelastic_setPeriod(lua_State* L) {
        auto self = Usertype<cocos2d::CCEaseElastic>::check(L, 1, "CCEaseElastic:setPeriod");
        assertMainThread();
        self->setPeriod(check<float>(L, 2, "CCEaseElastic:setPeriod"));
        return 0;
    }

    // Caller must leave the geode.cocos2d.actions table on top of the stack.
    void exposeFactoryTable(lua_State* L, char const* name, lua_CFunction createFn) {
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, createFn, name);
        lua_setfield(L, -2, "create");
        lua_setfield(L, -2, name);
    }

    void bindCCAction(lua_State* L) {
        Usertype<cocos2d::CCAction>::registerType(L, "CCAction", { Usertype<cocos2d::CCObject>::tag() });
        Usertype<cocos2d::CCAction>::method(L, "getTag", &ccaction_getTag);
        Usertype<cocos2d::CCAction>::method(L, "setTag", &ccaction_setTag);
        Usertype<cocos2d::CCAction>::method(L, "isDone", &ccaction_isDone);
        Usertype<cocos2d::CCAction>::method(L, "retain", &ccaction_retain);
        Usertype<cocos2d::CCAction>::method(L, "release", &ccaction_release);
        Usertype<cocos2d::CCAction>::method(L, "retainCount", &ccaction_retainCount);

        Usertype<cocos2d::CCFiniteTimeAction>::registerType(L, "CCFiniteTimeAction", { Usertype<cocos2d::CCAction>::tag() });
        Usertype<cocos2d::CCFiniteTimeAction>::method(L, "getDuration", &ccfinitetimeaction_getDuration);
        Usertype<cocos2d::CCFiniteTimeAction>::method(L, "setDuration", &ccfinitetimeaction_setDuration);

        Usertype<cocos2d::CCActionInterval>::registerType(L, "CCActionInterval", { Usertype<cocos2d::CCFiniteTimeAction>::tag() });
        Usertype<cocos2d::CCActionInterval>::method(L, "getElapsed", &ccactioninterval_getElapsed);

        Usertype<cocos2d::CCMoveTo>::registerType(L, "CCMoveTo", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCMoveBy>::registerType(L, "CCMoveBy", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCScaleTo>::registerType(L, "CCScaleTo", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCFadeIn>::registerType(L, "CCFadeIn", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCFadeOut>::registerType(L, "CCFadeOut", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCDelayTime>::registerType(L, "CCDelayTime", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCSequence>::registerType(L, "CCSequence", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCRepeatForever>::registerType(L, "CCRepeatForever", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCActionEase>::registerType(L, "CCActionEase", { Usertype<cocos2d::CCActionInterval>::tag() });
        Usertype<cocos2d::CCEaseRateAction>::registerType(L, "CCEaseRateAction", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseRateAction>::method(L, "getRate", &cceaserateaction_getRate);
        Usertype<cocos2d::CCEaseRateAction>::method(L, "setRate", &cceaserateaction_setRate);
        Usertype<cocos2d::CCEaseIn>::registerType(L, "CCEaseIn", { Usertype<cocos2d::CCEaseRateAction>::tag() });
        Usertype<cocos2d::CCEaseOut>::registerType(L, "CCEaseOut", { Usertype<cocos2d::CCEaseRateAction>::tag() });
        Usertype<cocos2d::CCEaseInOut>::registerType(L, "CCEaseInOut", { Usertype<cocos2d::CCEaseRateAction>::tag() });
        Usertype<cocos2d::CCEaseExponentialIn>::registerType(L, "CCEaseExponentialIn", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseExponentialOut>::registerType(L, "CCEaseExponentialOut", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseExponentialInOut>::registerType(L, "CCEaseExponentialInOut", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseSineIn>::registerType(L, "CCEaseSineIn", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseSineOut>::registerType(L, "CCEaseSineOut", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseSineInOut>::registerType(L, "CCEaseSineInOut", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseElastic>::registerType(L, "CCEaseElastic", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseElastic>::method(L, "getPeriod", &cceaseelastic_getPeriod);
        Usertype<cocos2d::CCEaseElastic>::method(L, "setPeriod", &cceaseelastic_setPeriod);
        Usertype<cocos2d::CCEaseElasticIn>::registerType(L, "CCEaseElasticIn", { Usertype<cocos2d::CCEaseElastic>::tag() });
        Usertype<cocos2d::CCEaseElasticOut>::registerType(L, "CCEaseElasticOut", { Usertype<cocos2d::CCEaseElastic>::tag() });
        Usertype<cocos2d::CCEaseElasticInOut>::registerType(L, "CCEaseElasticInOut", { Usertype<cocos2d::CCEaseElastic>::tag() });
        Usertype<cocos2d::CCEaseBounce>::registerType(L, "CCEaseBounce", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseBounceIn>::registerType(L, "CCEaseBounceIn", { Usertype<cocos2d::CCEaseBounce>::tag() });
        Usertype<cocos2d::CCEaseBounceOut>::registerType(L, "CCEaseBounceOut", { Usertype<cocos2d::CCEaseBounce>::tag() });
        Usertype<cocos2d::CCEaseBounceInOut>::registerType(L, "CCEaseBounceInOut", { Usertype<cocos2d::CCEaseBounce>::tag() });
        Usertype<cocos2d::CCEaseBackIn>::registerType(L, "CCEaseBackIn", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseBackOut>::registerType(L, "CCEaseBackOut", { Usertype<cocos2d::CCActionEase>::tag() });
        Usertype<cocos2d::CCEaseBackInOut>::registerType(L, "CCEaseBackInOut", { Usertype<cocos2d::CCActionEase>::tag() });

        getOrCreateTable(L, "geode.cocos2d.actions");
        exposeFactoryTable(L, "CCMoveTo", &ccmoveto_create);
        exposeFactoryTable(L, "CCMoveBy", &ccmoveby_create);
        exposeFactoryTable(L, "CCScaleTo", &ccscaleto_create);
        exposeFactoryTable(L, "CCFadeIn", &ccfadein_create);
        exposeFactoryTable(L, "CCFadeOut", &ccfadeout_create);
        exposeFactoryTable(L, "CCDelayTime", &ccdelaytime_create);
        exposeFactoryTable(L, "CCSequence", &ccsequence_create);
        exposeFactoryTable(L, "CCRepeatForever", &ccrepeatforever_create);
        exposeFactoryTable(L, "CCActionEase", &ccease_create<cocos2d::CCActionEase>);
        exposeFactoryTable(L, "CCEaseRateAction", &cceaserate_create<cocos2d::CCEaseRateAction>);
        exposeFactoryTable(L, "CCEaseIn", &cceaserate_create<cocos2d::CCEaseIn>);
        exposeFactoryTable(L, "CCEaseOut", &cceaserate_create<cocos2d::CCEaseOut>);
        exposeFactoryTable(L, "CCEaseInOut", &cceaserate_create<cocos2d::CCEaseInOut>);
        exposeFactoryTable(L, "CCEaseExponentialIn", &ccease_create<cocos2d::CCEaseExponentialIn>);
        exposeFactoryTable(L, "CCEaseExponentialOut", &ccease_create<cocos2d::CCEaseExponentialOut>);
        exposeFactoryTable(L, "CCEaseExponentialInOut", &ccease_create<cocos2d::CCEaseExponentialInOut>);
        exposeFactoryTable(L, "CCEaseSineIn", &ccease_create<cocos2d::CCEaseSineIn>);
        exposeFactoryTable(L, "CCEaseSineOut", &ccease_create<cocos2d::CCEaseSineOut>);
        exposeFactoryTable(L, "CCEaseSineInOut", &ccease_create<cocos2d::CCEaseSineInOut>);
        exposeFactoryTable(L, "CCEaseElastic", &cceaseelastic_create<cocos2d::CCEaseElastic>);
        exposeFactoryTable(L, "CCEaseElasticIn", &cceaseelastic_create<cocos2d::CCEaseElasticIn>);
        exposeFactoryTable(L, "CCEaseElasticOut", &cceaseelastic_create<cocos2d::CCEaseElasticOut>);
        exposeFactoryTable(L, "CCEaseElasticInOut", &cceaseelastic_create<cocos2d::CCEaseElasticInOut>);
        exposeFactoryTable(L, "CCEaseBounce", &ccease_create<cocos2d::CCEaseBounce>);
        exposeFactoryTable(L, "CCEaseBounceIn", &ccease_create<cocos2d::CCEaseBounceIn>);
        exposeFactoryTable(L, "CCEaseBounceOut", &ccease_create<cocos2d::CCEaseBounceOut>);
        exposeFactoryTable(L, "CCEaseBounceInOut", &ccease_create<cocos2d::CCEaseBounceInOut>);
        exposeFactoryTable(L, "CCEaseBackIn", &ccease_create<cocos2d::CCEaseBackIn>);
        exposeFactoryTable(L, "CCEaseBackOut", &ccease_create<cocos2d::CCEaseBackOut>);
        exposeFactoryTable(L, "CCEaseBackInOut", &ccease_create<cocos2d::CCEaseBackInOut>);
        lua_pop(L, 1);
    }

    LUAX_BINDING_PRIORITY(CCAction, bindCCAction, 2)
}
