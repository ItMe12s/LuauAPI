#include "../Binding.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include <cocos2d.h>
#include <lua.h>

namespace {
    using namespace luax;
    using Overlay = geode::OverlayManager;

    int overlay_get(lua_State* L) {
        Usertype<Overlay>::pushBorrowed(L, Overlay::get());
        return 1;
    }

    void bindOverlayManager(lua_State* L) {
        Usertype<Overlay>::registerType(L, "OverlayManager", { Usertype<cocos2d::CCNode>::tag() });

        getOrCreateTable(L, "geode");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &overlay_get, "OverlayManager.get");
        lua_setfield(L, -2, "get");
        lua_setfield(L, -2, "OverlayManager");
        lua_pop(L, 1);
    }

    LUAX_BINDING(OverlayManager, bindOverlayManager)
}
