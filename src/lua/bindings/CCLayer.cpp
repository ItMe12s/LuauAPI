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
    using Layer = cocos2d::CCLayer;

    int cclayer_create(lua_State* L) {
        assertMainThread();
        Usertype<Layer>::pushOwned(L, Layer::create());
        return 1;
    }

    void bindCCLayer(lua_State* L) {
        Usertype<Layer>::registerType(L, "CCLayer", { Usertype<cocos2d::CCNode>::tag() });

        getOrCreateTable(L, "geode.cocos2d");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &cclayer_create, "CCLayer.create");
        lua_setfield(L, -2, "create");
        lua_setfield(L, -2, "CCLayer");
        lua_pop(L, 1);
    }

    LUAX_BINDING(CCLayer, bindCCLayer)
}
