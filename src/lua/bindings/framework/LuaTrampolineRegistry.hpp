#pragma once

#include <cocos2d.h>

namespace luax {
    void anchorTrampoline(cocos2d::CCObject* anchor, cocos2d::CCObject* trampoline);
    void registerOrphanTrampoline(cocos2d::CCObject* trampoline);
    void evictTrampolinesIfFinalRelease(cocos2d::CCObject* anchor);
    void clearOrphanTrampolines();
    void ensureTrampolineShutdownHook();
}
