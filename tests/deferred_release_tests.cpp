#include "core/Runtime.hpp"
#include "framework/usertype/DeferredRelease.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cocos2d.h>
#include <thread>

namespace {
    struct DeferGuard {
        DeferGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::Runtime::setShuttingDownForTests(false);
            luax::drainDeferredReleases();
        }

        ~DeferGuard() {
            luax::Runtime::setShuttingDownForTests(false);
            luax::drainDeferredReleases();
            luax::Runtime::resetForTests();
        }
    };
} // namespace

TEST_CASE("deferred owned release is held until drain") {
    DeferGuard guard;

    auto* obj = new cocos2d::CCObject();
    REQUIRE(geode::detail::isLiveCocosObject(obj));

    luax::deferOwnedRelease(obj);
    REQUIRE(geode::detail::isLiveCocosObject(obj));

    luax::drainDeferredReleases();
    REQUIRE_FALSE(geode::detail::isLiveCocosObject(obj));
}

TEST_CASE("deferred release leaks instead of releasing during shutdown") {
    DeferGuard guard;

    auto* obj = new cocos2d::CCObject();
    luax::deferOwnedRelease(obj);

    luax::Runtime::setShuttingDownForTests(true);
    luax::drainDeferredReleases();
    REQUIRE(geode::detail::isLiveCocosObject(obj));

    luax::Runtime::setShuttingDownForTests(false);
    obj->release();
    REQUIRE_FALSE(geode::detail::isLiveCocosObject(obj));
}
