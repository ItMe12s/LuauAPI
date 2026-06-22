#include "core/Runtime.hpp"
#include "framework/usertype/DeferredRelease.hpp"

#include <Geode/utils/cocos.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cocos2d.h>
#include <thread>
#include <vector>

namespace {
    struct DeferGuard {
        explicit DeferGuard(bool pool = false) {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::Runtime::setShuttingDownForTests(false);
            geode::detail::weakRefSimulateForgetForTests() = true;
            geode::detail::weakRefSimulatePoolForTests() = pool;
            geode::detail::weakRefLockRetainsForTests() = false;
            luax::drainDeferredReleases();
        }

        ~DeferGuard() {
            luax::Runtime::setShuttingDownForTests(false);
            geode::detail::weakRefSimulateForgetForTests() = false;
            geode::detail::weakRefSimulatePoolForTests() = false;
            geode::detail::weakRefLockRetainsForTests() = false;
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

TEST_CASE("deferred owned release keepalive survives user release until drain") {
    DeferGuard guard;

    auto* node = new cocos2d::CCNode();
    luax::deferOwnedRelease(node);
    node->release();
    REQUIRE(geode::detail::isLiveCocosObject(node));

    luax::drainDeferredReleases();
    REQUIRE_FALSE(geode::detail::isLiveCocosObject(node));
}

TEST_CASE("owned drain applies one logical release with queue keepalive") {
    DeferGuard guard;
    geode::detail::weakRefLockRetainsForTests() = true;

    auto* obj = new cocos2d::CCObject();
    obj->retain();
    REQUIRE(obj->retainCount() == 2);

    luax::deferOwnedRelease(obj);
    REQUIRE(obj->retainCount() == 3);
    REQUIRE(obj->releaseCallCount() == 0);

    luax::drainDeferredReleases();
    REQUIRE(obj->releaseCallCount() == 2);
    REQUIRE(obj->retainCount() == 1);

    obj->release();
    REQUIRE_FALSE(geode::detail::isLiveCocosObject(obj));
}

TEST_CASE("borrowed and owned same object with pool-like WeakRef") {
    DeferGuard guard(true);

    auto* obj = new cocos2d::CCObject();
    obj->retain();
    REQUIRE(obj->retainCount() == 2);

    luax::deferBorrowedRelease(geode::WeakRef<cocos2d::CCObject>(obj));
    luax::deferOwnedRelease(obj);
    REQUIRE(obj->retainCount() == 5);

    luax::drainDeferredReleases();
    REQUIRE(geode::detail::isLiveCocosObject(obj));
    REQUIRE(obj->retainCount() == 3);

    obj->release();
    obj->release();
    obj->release();
    REQUIRE_FALSE(geode::detail::isLiveCocosObject(obj));
}
