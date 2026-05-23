#include "../Binding.hpp"
#include "internal/Ref.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Types.hpp"
#include "internal/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>

namespace {
    using namespace luax;
    using GM = GameManager;

    int gm_get(lua_State* L) {
        auto* gm = GM::get();
        if (!gm) {
            luaL_error(L, "GameManager.get: singleton not available");
        }
        Usertype<GM>::pushBorrowed(L, gm);
        return 1;
    }

    int gm_getPlayerFrame(lua_State* L) {
        auto self = Usertype<GM>::check(L, 1, "GameManager:getPlayerFrame");
        push(L, self->getPlayerFrame());
        return 1;
    }

    int gm_getPlayerColor(lua_State* L) {
        auto self = Usertype<GM>::check(L, 1, "GameManager:getPlayerColor");
        push(L, self->getPlayerColor());
        return 1;
    }

    int gm_getPlayerColor2(lua_State* L) {
        auto self = Usertype<GM>::check(L, 1, "GameManager:getPlayerColor2");
        push(L, self->getPlayerColor2());
        return 1;
    }

    int gm_getPlayerGlow(lua_State* L) {
        auto self = Usertype<GM>::check(L, 1, "GameManager:getPlayerGlow");
        push(L, self->getPlayerGlow());
        return 1;
    }

    int gm_getPlayerGlowColor(lua_State* L) {
        auto self = Usertype<GM>::check(L, 1, "GameManager:getPlayerGlowColor");
        push(L, self->getPlayerGlowColor());
        return 1;
    }

    int gm_colorForIdx(lua_State* L) {
        auto self = Usertype<GM>::check(L, 1, "GameManager:colorForIdx");
        assertMainThread();
        pushColor3B(L, self->colorForIdx(check<int>(L, 2, "GameManager:colorForIdx")));
        return 1;
    }

    void bindGameManager(lua_State* L) {
        Usertype<GM>::registerType(L, "GameManager", { Usertype<cocos2d::CCNode>::tag() });

        Usertype<GM>::method(L, "getPlayerFrame",     &gm_getPlayerFrame);
        Usertype<GM>::method(L, "getPlayerColor",     &gm_getPlayerColor);
        Usertype<GM>::method(L, "getPlayerColor2",    &gm_getPlayerColor2);
        Usertype<GM>::method(L, "getPlayerGlow",      &gm_getPlayerGlow);
        Usertype<GM>::method(L, "getPlayerGlowColor", &gm_getPlayerGlowColor);
        Usertype<GM>::method(L, "colorForIdx",        &gm_colorForIdx);

        getOrCreateTable(L, "geode");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &gm_get, "GameManager.get");
        lua_setfield(L, -2, "get");
        lua_setfield(L, -2, "GameManager");
        lua_pop(L, 1);
    }

    LUAX_BINDING(GameManager, bindGameManager)
}
