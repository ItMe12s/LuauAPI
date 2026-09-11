#pragma once

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <cstdint>
#include <lua.h>

namespace luax {
    void adoptDeferredReleaseThread();
    void clearDeferredReleases();
    void deferBorrowedRelease(geode::WeakRef<cocos2d::CCObject>&& weak);
    void deferOwnedRelease(cocos2d::CCObject* obj);
    void deferLuaRefUnref(lua_State* state, int ref, std::uint32_t generation);
    void drainDeferredReleases();
} // namespace luax
