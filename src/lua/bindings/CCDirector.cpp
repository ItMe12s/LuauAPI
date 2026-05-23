#include "../Binding.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Types.hpp"
#include "internal/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>

namespace {
    using namespace luax;
    using Director = cocos2d::CCDirector;

    int director_shared(lua_State* L) {
        Usertype<Director>::pushBorrowed(L, Director::sharedDirector());
        return 1;
    }

    int director_getWinSize(lua_State* L) {
        auto self = Usertype<Director>::check(L, 1, "CCDirector:getWinSize");
        pushSize(L, self->getWinSize());
        return 1;
    }
    int director_getVisibleOrigin(lua_State* L) {
        auto self = Usertype<Director>::check(L, 1, "CCDirector:getVisibleOrigin");
        pushPoint(L, self->getVisibleOrigin());
        return 1;
    }
    int director_getVisibleSize(lua_State* L) {
        auto self = Usertype<Director>::check(L, 1, "CCDirector:getVisibleSize");
        pushSize(L, self->getVisibleSize());
        return 1;
    }

    void bindCCDirector(lua_State* L) {
        Usertype<Director>::registerType(L, "CCDirector", { Usertype<cocos2d::CCObject>::tag() });
        Usertype<Director>::method(L, "getWinSize",       &director_getWinSize);
        Usertype<Director>::method(L, "getVisibleOrigin", &director_getVisibleOrigin);
        Usertype<Director>::method(L, "getVisibleSize",   &director_getVisibleSize);

        getOrCreateTable(L, "geode.cocos2d");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &director_shared, "CCDirector.sharedDirector");
        lua_setfield(L, -2, "sharedDirector");
        lua_setfield(L, -2, "CCDirector");
        lua_pop(L, 1);
    }

    LUAX_BINDING(CCDirector, bindCCDirector)
}
