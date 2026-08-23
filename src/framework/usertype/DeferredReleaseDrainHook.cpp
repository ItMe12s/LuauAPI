#include "framework/usertype/DeferredRelease.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>

using namespace geode::prelude;

class $modify(LuauAPIDrainDirector, cocos2d::CCDirector) {
public:
    static void onModify(auto& self) {
        if (!self.setHookPriorityPost("cocos2d::CCDirector::drawScene", Priority::Late)) {
            log::warn("Failed to set hook priority for deferred release drain");
        }
    }

    void drawScene() {
        luax::adoptDeferredReleaseThread();
        CCDirector::drawScene();
        luax::adoptDeferredReleaseThread();
        luax::drainDeferredReleases();
    }
};
