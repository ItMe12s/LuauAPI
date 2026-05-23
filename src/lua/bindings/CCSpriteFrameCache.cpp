#include "../Binding.hpp"
#include "internal/Ref.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <string>

namespace {
    using namespace luax;
    using Cache = cocos2d::CCSpriteFrameCache;

    int cache_shared(lua_State* L) {
        Usertype<Cache>::pushBorrowed(L, Cache::sharedSpriteFrameCache());
        return 1;
    }

    int cache_addSpriteFramesWithFile(lua_State* L) {
        auto self = Usertype<Cache>::check(L, 1, "CCSpriteFrameCache:addSpriteFramesWithFile");
        auto plist = check<std::string>(L, 2, "CCSpriteFrameCache:addSpriteFramesWithFile");
        assertMainThread();
        self->addSpriteFramesWithFile(plist.c_str());
        return 0;
    }

    int cache_spriteFrameByName(lua_State* L) {
        auto self = Usertype<Cache>::check(L, 1, "CCSpriteFrameCache:spriteFrameByName");
        auto name = check<std::string>(L, 2, "CCSpriteFrameCache:spriteFrameByName");
        Usertype<cocos2d::CCSpriteFrame>::pushBorrowed(L, self->spriteFrameByName(name.c_str()));
        return 1;
    }

    void bindCCSpriteFrameCache(lua_State* L) {
        Usertype<Cache>::registerType(L, "CCSpriteFrameCache", { Usertype<cocos2d::CCObject>::tag() });
        Usertype<Cache>::method(L, "addSpriteFramesWithFile", &cache_addSpriteFramesWithFile);
        Usertype<Cache>::method(L, "spriteFrameByName", &cache_spriteFrameByName);

        getOrCreateTable(L, "geode.cocos2d");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &cache_shared, "CCSpriteFrameCache.sharedSpriteFrameCache");
        lua_setfield(L, -2, "sharedSpriteFrameCache");
        lua_setfield(L, -2, "CCSpriteFrameCache");
        lua_pop(L, 1);
    }

    LUAX_BINDING(CCSpriteFrameCache, bindCCSpriteFrameCache)
}
