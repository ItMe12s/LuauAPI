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

namespace {
    using namespace luax;
    using Sprite = cocos2d::CCSprite;

    int ccsprite_create(lua_State* L) {
        assertMainThread();
        Usertype<Sprite>::pushOwned(L, Sprite::create());
        return 1;
    }

    int ccsprite_createWithSpriteFrameName(lua_State* L) {
        assertMainThread();
        auto name = check<std::string>(L, 1, "CCSprite:createWithSpriteFrameName");
        Usertype<Sprite>::pushOwned(L, Sprite::createWithSpriteFrameName(name.c_str()));
        return 1;
    }

    int ccsprite_createWithSpriteFrame(lua_State* L) {
        assertMainThread();
        auto frame = Usertype<cocos2d::CCSpriteFrame>::check(L, 1, "CCSprite:createWithSpriteFrame");
        Usertype<Sprite>::pushOwned(L, Sprite::createWithSpriteFrame(
            requireSpriteFrame(frame, "CCSprite:createWithSpriteFrame")));
        return 1;
    }

    int ccsprite_setFlipX(lua_State* L) {
        auto self = Usertype<Sprite>::check(L, 1, "CCSprite:setFlipX");
        assertMainThread();
        self->setFlipX(check<bool>(L, 2, "CCSprite:setFlipX"));
        return 0;
    }
    int ccsprite_setFlipY(lua_State* L) {
        auto self = Usertype<Sprite>::check(L, 1, "CCSprite:setFlipY");
        assertMainThread();
        self->setFlipY(check<bool>(L, 2, "CCSprite:setFlipY"));
        return 0;
    }
    int ccsprite_isFlipX(lua_State* L) {
        push(L, Usertype<Sprite>::check(L, 1, "CCSprite:isFlipX")->isFlipX());
        return 1;
    }
    int ccsprite_isFlipY(lua_State* L) {
        push(L, Usertype<Sprite>::check(L, 1, "CCSprite:isFlipY")->isFlipY());
        return 1;
    }
    int ccsprite_setDisplayFrame(lua_State* L) {
        auto self = Usertype<Sprite>::check(L, 1, "CCSprite:setDisplayFrame");
        auto frame = Usertype<cocos2d::CCSpriteFrame>::check(L, 2, "CCSprite:setDisplayFrame");
        assertMainThread();
        self->setDisplayFrame(requireSpriteFrame(frame, "CCSprite:setDisplayFrame"));
        return 0;
    }
    int ccsprite_setAliasTexParameters(lua_State* L) {
        auto self = Usertype<Sprite>::check(L, 1, "CCSprite:setAliasTexParameters");
        assertMainThread();
        if (auto texture = self->getTexture()) {
            texture->setAliasTexParameters();
        }
        return 0;
    }

    void bindCCSprite(lua_State* L) {
        Usertype<Sprite>::registerType(L, "CCSprite", { Usertype<cocos2d::CCNode>::tag() });
        Usertype<Sprite>::method(L, "setFlipX",              &ccsprite_setFlipX);
        Usertype<Sprite>::method(L, "setFlipY",              &ccsprite_setFlipY);
        Usertype<Sprite>::method(L, "isFlipX",               &ccsprite_isFlipX);
        Usertype<Sprite>::method(L, "isFlipY",               &ccsprite_isFlipY);
        Usertype<Sprite>::method(L, "setDisplayFrame",       &ccsprite_setDisplayFrame);
        Usertype<Sprite>::method(L, "setAliasTexParameters", &ccsprite_setAliasTexParameters);

        getOrCreateTable(L, "geode.cocos2d");
        lua_createtable(L, 0, 3);
        lua_pushcfunction(L, &ccsprite_create, "CCSprite.create");
        lua_setfield(L, -2, "create");
        lua_pushcfunction(L, &ccsprite_createWithSpriteFrameName, "CCSprite.createWithSpriteFrameName");
        lua_setfield(L, -2, "createWithSpriteFrameName");
        lua_pushcfunction(L, &ccsprite_createWithSpriteFrame, "CCSprite.createWithSpriteFrame");
        lua_setfield(L, -2, "createWithSpriteFrame");
        lua_setfield(L, -2, "CCSprite");
        lua_pop(L, 1);
    }

    LUAX_BINDING(CCSprite, bindCCSprite)
}
