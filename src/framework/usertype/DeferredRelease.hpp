#pragma once

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>

namespace luax {
    void adoptDeferredReleaseThread();
    void clearDeferredReleases();
    void deferBorrowedRelease(geode::WeakRef<cocos2d::CCObject>&& weak);
    void deferOwnedRelease(cocos2d::CCObject* obj);
    void drainDeferredReleases();
} // namespace luax
