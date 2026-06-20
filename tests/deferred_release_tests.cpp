#include "core/Runtime.hpp"
#include "framework/usertype/DeferredRelease.hpp"

#include <Geode/utils/cocos.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cocos2d.h>
#include <thread>
#include <vector>

namespace {
    struct DeferGuard {
        DeferGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::Runtime::setShuttingDownForTests(false);
            geode::detail::weakRefSimulateForgetForTests() = true;
            luax::drainDeferredReleases();
        }

        ~DeferGuard() {
            luax::Runtime::setShuttingDownForTests(false);
            geode::detail::weakRefSimulateForgetForTests() = false;
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

TEST_CASE("bulk deferred releases are all held until drain") {
    DeferGuard guard;

    constexpr int kCount = 256;
    std::vector<cocos2d::CCObject*> objects;
    objects.reserve(kCount);
    for (int i = 0; i < kCount; ++i) {
        auto* obj = new cocos2d::CCObject();
        objects.push_back(obj);
        luax::deferBorrowedRelease(geode::WeakRef<cocos2d::CCObject>(obj));
        luax::deferOwnedRelease(obj);
    }

    for (auto* obj : objects) {
        REQUIRE(geode::detail::isLiveCocosObject(obj));
    }

    luax::drainDeferredReleases();

    for (auto* obj : objects) {
        REQUIRE_FALSE(geode::detail::isLiveCocosObject(obj));
    }
}

TEST_CASE("duplicate owned deferred releases do not double-free") {
    DeferGuard guard;

    auto* obj = new cocos2d::CCObject();
    luax::deferOwnedRelease(obj);
    luax::deferOwnedRelease(obj);
    REQUIRE(geode::detail::isLiveCocosObject(obj));

    luax::drainDeferredReleases();
    REQUIRE_FALSE(geode::detail::isLiveCocosObject(obj));
}

TEST_CASE("deferred owned release skips stale object at drain") {
    DeferGuard guard;

    auto* obj = new cocos2d::CCObject();
    luax::deferOwnedRelease(obj);
    obj->release();
    REQUIRE_FALSE(geode::detail::isLiveCocosObject(obj));

    REQUIRE_NOTHROW(luax::drainDeferredReleases());
}
