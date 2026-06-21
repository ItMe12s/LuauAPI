#include "framework/usertype/DeferredRelease.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>

using namespace geode::prelude;

class $modify(LuauAPIDrainDirector, cocos2d::CCDirector) {
public:
    void drawScene() {
        CCDirector::drawScene();
        luax::drainDeferredReleases();
    }
};
