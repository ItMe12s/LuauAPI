#include "../Binding.hpp"
#include "internal/Ref.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Usertype.hpp"
#include "internal/Validate.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <string>
#include <string_view>

namespace {
    using namespace luax;
    using Node = cocos2d::CCNode;

    int ccnode_addChild(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:addChild");
        auto child = Usertype<Node>::check(L, 2, "CCNode:addChild");
        assertMainThread();
        requireNode(child, "CCNode:addChild");
        int top = lua_gettop(L);
        if (top == 2) self->addChild(child);
        else if (top == 3) self->addChild(child, check<int>(L, 3, "CCNode:addChild"));
        else self->addChild(child, check<int>(L, 3, "CCNode:addChild"), check<int>(L, 4, "CCNode:addChild"));
        return 0;
    }

    int ccnode_removeChild(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:removeChild");
        auto child = Usertype<Node>::check(L, 2, "CCNode:removeChild");
        assertMainThread();
        requireNode(child, "CCNode:removeChild");
        bool cleanup = lua_gettop(L) >= 3 ? check<bool>(L, 3, "CCNode:removeChild") : true;
        self->removeChild(child, cleanup);
        return 0;
    }

    int ccnode_removeChildByTag(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:removeChildByTag");
        assertMainThread();
        int tag = check<int>(L, 2, "CCNode:removeChildByTag");
        bool cleanup = lua_gettop(L) >= 3 ? check<bool>(L, 3, "CCNode:removeChildByTag") : true;
        self->removeChildByTag(tag, cleanup);
        return 0;
    }

    int ccnode_removeAllChildrenWithCleanup(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:removeAllChildrenWithCleanup");
        assertMainThread();
        self->removeAllChildrenWithCleanup(check<bool>(L, 2, "CCNode:removeAllChildrenWithCleanup"));
        return 0;
    }

    int ccnode_removeFromParentAndCleanup(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:removeFromParentAndCleanup");
        assertMainThread();
        self->removeFromParentAndCleanup(check<bool>(L, 2, "CCNode:removeFromParentAndCleanup"));
        return 0;
    }

    int ccnode_getChildByTag(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:getChildByTag");
        Usertype<Node>::pushBorrowed(L, self->getChildByTag(check<int>(L, 2, "CCNode:getChildByTag")));
        return 1;
    }

    int ccnode_getChildByID(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:getChildByID");
        std::string id = check<std::string>(L, 2, "CCNode:getChildByID");
        Usertype<Node>::pushBorrowed(L, self->getChildByID(id));
        return 1;
    }

    int ccnode_getChildrenCount(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:getChildrenCount");
        push(L, self->getChildrenCount());
        return 1;
    }

    int ccnode_getParent(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:getParent");
        Usertype<Node>::pushBorrowed(L, self->getParent());
        return 1;
    }

    int ccnode_setPosition(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setPosition");
        assertMainThread();
        self->setPosition({ check<float>(L, 2, "CCNode:setPosition"), check<float>(L, 3, "CCNode:setPosition") });
        return 0;
    }

    int ccnode_getPositionX(lua_State* L) { push(L, Usertype<Node>::check(L, 1, "CCNode:getPositionX")->getPositionX()); return 1; }
    int ccnode_getPositionY(lua_State* L) { push(L, Usertype<Node>::check(L, 1, "CCNode:getPositionY")->getPositionY()); return 1; }

    int ccnode_setPositionX(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setPositionX");
        assertMainThread();
        self->setPositionX(check<float>(L, 2, "CCNode:setPositionX"));
        return 0;
    }
    int ccnode_setPositionY(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setPositionY");
        assertMainThread();
        self->setPositionY(check<float>(L, 2, "CCNode:setPositionY"));
        return 0;
    }

    int ccnode_getScale(lua_State* L)  { push(L, Usertype<Node>::check(L, 1, "CCNode:getScale")->getScale()); return 1; }
    int ccnode_getScaleX(lua_State* L) { push(L, Usertype<Node>::check(L, 1, "CCNode:getScaleX")->getScaleX()); return 1; }
    int ccnode_getScaleY(lua_State* L) { push(L, Usertype<Node>::check(L, 1, "CCNode:getScaleY")->getScaleY()); return 1; }

    int ccnode_setScale(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setScale");
        assertMainThread();
        self->setScale(check<float>(L, 2, "CCNode:setScale"));
        return 0;
    }
    int ccnode_setScaleX(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setScaleX");
        assertMainThread();
        self->setScaleX(check<float>(L, 2, "CCNode:setScaleX"));
        return 0;
    }
    int ccnode_setScaleY(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setScaleY");
        assertMainThread();
        self->setScaleY(check<float>(L, 2, "CCNode:setScaleY"));
        return 0;
    }

    int ccnode_getRotation(lua_State* L) {
        push(L, Usertype<Node>::check(L, 1, "CCNode:getRotation")->getRotation());
        return 1;
    }
    int ccnode_setRotation(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setRotation");
        assertMainThread();
        self->setRotation(check<float>(L, 2, "CCNode:setRotation"));
        return 0;
    }

    int ccnode_getAnchorPoint(lua_State* L) {
        auto& p = Usertype<Node>::check(L, 1, "CCNode:getAnchorPoint")->getAnchorPoint();
        push(L, p.x); push(L, p.y);
        return 2;
    }
    int ccnode_setAnchorPoint(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setAnchorPoint");
        assertMainThread();
        self->setAnchorPoint({ check<float>(L, 2, "CCNode:setAnchorPoint"), check<float>(L, 3, "CCNode:setAnchorPoint") });
        return 0;
    }

    int ccnode_getContentSize(lua_State* L) {
        auto& s = Usertype<Node>::check(L, 1, "CCNode:getContentSize")->getContentSize();
        push(L, s.width); push(L, s.height);
        return 2;
    }
    int ccnode_setContentSize(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setContentSize");
        assertMainThread();
        self->setContentSize({ check<float>(L, 2, "CCNode:setContentSize"), check<float>(L, 3, "CCNode:setContentSize") });
        return 0;
    }

    int ccnode_isVisible(lua_State* L) { push(L, Usertype<Node>::check(L, 1, "CCNode:isVisible")->isVisible()); return 1; }
    int ccnode_setVisible(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setVisible");
        assertMainThread();
        self->setVisible(check<bool>(L, 2, "CCNode:setVisible"));
        return 0;
    }

    int ccnode_getZOrder(lua_State* L) { push(L, Usertype<Node>::check(L, 1, "CCNode:getZOrder")->getZOrder()); return 1; }
    int ccnode_setZOrder(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setZOrder");
        assertMainThread();
        self->setZOrder(check<int>(L, 2, "CCNode:setZOrder"));
        return 0;
    }

    int ccnode_getID(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:getID");
        push(L, std::string(std::string_view(self->getID())));
        return 1;
    }
    int ccnode_setID(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:setID");
        assertMainThread();
        self->setID(check<std::string>(L, 2, "CCNode:setID"));
        return 0;
    }

    int ccnode_isRunning(lua_State* L) { push(L, Usertype<Node>::check(L, 1, "CCNode:isRunning")->isRunning()); return 1; }

    int ccnode_runAction(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:runAction");
        auto action = Usertype<cocos2d::CCAction>::check(L, 2, "CCNode:runAction");
        assertMainThread();
        Usertype<cocos2d::CCAction>::pushBorrowed(L, self->runAction(requireAction(action, "CCNode:runAction")));
        return 1;
    }

    int ccnode_stopAllActions(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:stopAllActions");
        assertMainThread();
        self->stopAllActions();
        return 0;
    }

    int ccnode_scheduleUpdate(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:scheduleUpdate");
        assertMainThread();
        self->scheduleUpdate();
        return 0;
    }

    int ccnode_unscheduleUpdate(lua_State* L) {
        auto self = Usertype<Node>::check(L, 1, "CCNode:unscheduleUpdate");
        assertMainThread();
        self->unscheduleUpdate();
        return 0;
    }

    void bindCCNode(lua_State* L) {
        Usertype<Node>::registerType(L, "CCNode", { Usertype<cocos2d::CCObject>::tag() });

        Usertype<Node>::method(L, "addChild",                     &ccnode_addChild);
        Usertype<Node>::method(L, "removeChild",                  &ccnode_removeChild);
        Usertype<Node>::method(L, "removeChildByTag",             &ccnode_removeChildByTag);
        Usertype<Node>::method(L, "removeAllChildrenWithCleanup", &ccnode_removeAllChildrenWithCleanup);
        Usertype<Node>::method(L, "removeFromParentAndCleanup",   &ccnode_removeFromParentAndCleanup);
        Usertype<Node>::method(L, "getChildByTag",                &ccnode_getChildByTag);
        Usertype<Node>::method(L, "getChildByID",                 &ccnode_getChildByID);
        Usertype<Node>::method(L, "getChildrenCount",             &ccnode_getChildrenCount);
        Usertype<Node>::method(L, "getParent",                    &ccnode_getParent);
        Usertype<Node>::method(L, "setPosition",                  &ccnode_setPosition);
        Usertype<Node>::method(L, "getPositionX",                 &ccnode_getPositionX);
        Usertype<Node>::method(L, "getPositionY",                 &ccnode_getPositionY);
        Usertype<Node>::method(L, "setPositionX",                 &ccnode_setPositionX);
        Usertype<Node>::method(L, "setPositionY",                 &ccnode_setPositionY);
        Usertype<Node>::method(L, "getScale",                     &ccnode_getScale);
        Usertype<Node>::method(L, "setScale",                     &ccnode_setScale);
        Usertype<Node>::method(L, "getScaleX",                    &ccnode_getScaleX);
        Usertype<Node>::method(L, "getScaleY",                    &ccnode_getScaleY);
        Usertype<Node>::method(L, "setScaleX",                    &ccnode_setScaleX);
        Usertype<Node>::method(L, "setScaleY",                    &ccnode_setScaleY);
        Usertype<Node>::method(L, "getRotation",                  &ccnode_getRotation);
        Usertype<Node>::method(L, "setRotation",                  &ccnode_setRotation);
        Usertype<Node>::method(L, "getAnchorPoint",               &ccnode_getAnchorPoint);
        Usertype<Node>::method(L, "setAnchorPoint",               &ccnode_setAnchorPoint);
        Usertype<Node>::method(L, "getContentSize",               &ccnode_getContentSize);
        Usertype<Node>::method(L, "setContentSize",               &ccnode_setContentSize);
        Usertype<Node>::method(L, "isVisible",                    &ccnode_isVisible);
        Usertype<Node>::method(L, "setVisible",                   &ccnode_setVisible);
        Usertype<Node>::method(L, "getZOrder",                    &ccnode_getZOrder);
        Usertype<Node>::method(L, "setZOrder",                    &ccnode_setZOrder);
        Usertype<Node>::method(L, "getID",                        &ccnode_getID);
        Usertype<Node>::method(L, "setID",                        &ccnode_setID);
        Usertype<Node>::method(L, "isRunning",                    &ccnode_isRunning);
        Usertype<Node>::method(L, "runAction",                    &ccnode_runAction);
        Usertype<Node>::method(L, "stopAllActions",               &ccnode_stopAllActions);
        Usertype<Node>::method(L, "scheduleUpdate",               &ccnode_scheduleUpdate);
        Usertype<Node>::method(L, "unscheduleUpdate",             &ccnode_unscheduleUpdate);

    }

    LUAX_BINDING_PRIORITY(CCNode, bindCCNode, 1)
}
