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
    using SP = SimplePlayer;

    // Mirrors geode.IconType below, keep in sync.
    bool isValidIconType(int v) {
        return (v >= 0 && v <= 8) || v == 98 || v == 99 || v == 100 || v == 101;
    }

    int simpleplayer_create(lua_State* L) {
        assertMainThread();
        int id = check<int>(L, 1, "SimplePlayer.create");
        auto* sp = SP::create(id);
        if (!sp) {
            luaL_error(L, "SimplePlayer.create: failed to create with iconId=%d", id);
        }
        Usertype<SP>::pushOwned(L, sp);
        return 1;
    }

    int simpleplayer_updatePlayerFrame(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:updatePlayerFrame");
        assertMainThread();
        int id = check<int>(L, 2, "SimplePlayer:updatePlayerFrame");
        int type = check<int>(L, 3, "SimplePlayer:updatePlayerFrame");
        if (!isValidIconType(type)) {
            luaL_error(L,
                "SimplePlayer:updatePlayerFrame: invalid iconType=%d "
                "(expected 0..8, 98, 99, 100, or 101 — see geode.IconType)",
                type);
        }
        self->updatePlayerFrame(id, static_cast<IconType>(type));
        return 0;
    }

    int simpleplayer_setColor(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:setColor");
        assertMainThread();
        if (lua_type(L, 2) != LUA_TTABLE) {
            luaL_error(L, "SimplePlayer:setColor expected color table");
        }
        self->setColor(toColor3B(L, 2, "SimplePlayer:setColor"));
        return 0;
    }

    int simpleplayer_setSecondColor(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:setSecondColor");
        assertMainThread();
        if (lua_type(L, 2) != LUA_TTABLE) {
            luaL_error(L, "SimplePlayer:setSecondColor expected color table");
        }
        self->setSecondColor(toColor3B(L, 2, "SimplePlayer:setSecondColor"));
        return 0;
    }

    int simpleplayer_setColors(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:setColors");
        assertMainThread();
        if (lua_type(L, 2) != LUA_TTABLE || lua_type(L, 3) != LUA_TTABLE) {
            luaL_error(L, "SimplePlayer:setColors expected two color tables");
        }
        self->setColors(
            toColor3B(L, 2, "SimplePlayer:setColors"),
            toColor3B(L, 3, "SimplePlayer:setColors")
        );
        return 0;
    }

    int simpleplayer_updateColors(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:updateColors");
        assertMainThread();
        self->updateColors();
        return 0;
    }

    int simpleplayer_hideSecondary(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:hideSecondary");
        assertMainThread();
        self->hideSecondary();
        return 0;
    }

    int simpleplayer_hideAll(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:hideAll");
        assertMainThread();
        self->hideAll();
        return 0;
    }

    int simpleplayer_setGlowOutline(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:setGlowOutline");
        assertMainThread();
        if (lua_type(L, 2) != LUA_TTABLE) {
            luaL_error(L, "SimplePlayer:setGlowOutline expected color table");
        }
        self->setGlowOutline(toColor3B(L, 2, "SimplePlayer:setGlowOutline"));
        return 0;
    }

    int simpleplayer_enableCustomGlowColor(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:enableCustomGlowColor");
        assertMainThread();
        if (lua_type(L, 2) != LUA_TTABLE) {
            luaL_error(L, "SimplePlayer:enableCustomGlowColor expected color table");
        }
        self->enableCustomGlowColor(toColor3B(L, 2, "SimplePlayer:enableCustomGlowColor"));
        return 0;
    }

    int simpleplayer_disableCustomGlowColor(lua_State* L) {
        auto self = Usertype<SP>::check(L, 1, "SimplePlayer:disableCustomGlowColor");
        assertMainThread();
        self->disableCustomGlowColor();
        return 0;
    }

    void bindSimplePlayer(lua_State* L) {
        Usertype<SP>::registerType(L, "SimplePlayer", { Usertype<cocos2d::CCSprite>::tag() });

        Usertype<SP>::method(L, "updatePlayerFrame",      &simpleplayer_updatePlayerFrame);
        Usertype<SP>::method(L, "setColor",               &simpleplayer_setColor);
        Usertype<SP>::method(L, "setSecondColor",         &simpleplayer_setSecondColor);
        Usertype<SP>::method(L, "setColors",              &simpleplayer_setColors);
        Usertype<SP>::method(L, "updateColors",           &simpleplayer_updateColors);
        Usertype<SP>::method(L, "hideSecondary",          &simpleplayer_hideSecondary);
        Usertype<SP>::method(L, "hideAll",                &simpleplayer_hideAll);
        Usertype<SP>::method(L, "setGlowOutline",         &simpleplayer_setGlowOutline);
        Usertype<SP>::method(L, "enableCustomGlowColor",  &simpleplayer_enableCustomGlowColor);
        Usertype<SP>::method(L, "disableCustomGlowColor", &simpleplayer_disableCustomGlowColor);

        getOrCreateTable(L, "geode");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &simpleplayer_create, "SimplePlayer.create");
        lua_setfield(L, -2, "create");
        lua_setfield(L, -2, "SimplePlayer");

        lua_createtable(L, 0, 13);
        lua_pushinteger(L, 0);   lua_setfield(L, -2, "Cube");
        lua_pushinteger(L, 1);   lua_setfield(L, -2, "Ship");
        lua_pushinteger(L, 2);   lua_setfield(L, -2, "Ball");
        lua_pushinteger(L, 3);   lua_setfield(L, -2, "Ufo");
        lua_pushinteger(L, 4);   lua_setfield(L, -2, "Wave");
        lua_pushinteger(L, 5);   lua_setfield(L, -2, "Robot");
        lua_pushinteger(L, 6);   lua_setfield(L, -2, "Spider");
        lua_pushinteger(L, 7);   lua_setfield(L, -2, "Swing");
        lua_pushinteger(L, 8);   lua_setfield(L, -2, "Jetpack");
        lua_pushinteger(L, 98);  lua_setfield(L, -2, "DeathEffect");
        lua_pushinteger(L, 99);  lua_setfield(L, -2, "Special");
        lua_pushinteger(L, 100); lua_setfield(L, -2, "Item");
        lua_pushinteger(L, 101); lua_setfield(L, -2, "ShipFire");
        lua_setfield(L, -2, "IconType");

        lua_pop(L, 1);
    }

    LUAX_BINDING(SimplePlayer, bindSimplePlayer)
}
