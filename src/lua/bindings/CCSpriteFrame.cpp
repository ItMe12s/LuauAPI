#include "../Binding.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>

namespace {
    using namespace luax;

    void bindCCSpriteFrame(lua_State* L) {
        Usertype<cocos2d::CCSpriteFrame>::registerType(L, "CCSpriteFrame", { Usertype<cocos2d::CCObject>::tag() });

        getOrCreateTable(L, "geode.cocos2d");
        lua_createtable(L, 0, 0);
        lua_setfield(L, -2, "CCSpriteFrame");
        lua_pop(L, 1);
    }

    LUAX_BINDING_PRIORITY(CCSpriteFrame, bindCCSpriteFrame, 2)
}
