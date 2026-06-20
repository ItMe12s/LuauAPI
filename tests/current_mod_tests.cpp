#include "bindings/geode/CurrentMod.hpp"
#include "core/Runtime.hpp"
#include "host/lua_test_helpers.hpp"

#include <Geode/loader/Mod.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

namespace {
    using RuntimeGuard = luauapi_test::ModRuntimeGuard;
} // namespace

TEST_CASE("currentMod returns nullptr without runtime") {
    geode::Mod::resetForTests();
    luax::invalidateCurrentModCache();
    REQUIRE(luax::currentMod() == nullptr);
}

TEST_CASE("currentMod returns nullptr with empty resources root") {
    RuntimeGuard guard;
    luax::Runtime::getOrCreate();
    REQUIRE(luax::currentMod() == nullptr);
}

TEST_CASE("currentMod resolves mod from resources root") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_current_mod_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);

    REQUIRE(luax::currentMod() == mod);
}

TEST_CASE("currentMod does not fall back to Mod::get") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dirA{"luauapi_current_mod_"};
    luauapi_test::ScopedTempDir dirB{"luauapi_current_mod_"};
    auto* modA = geode::Mod::create(dirA.path);
    auto* modB = geode::Mod::create(dirB.path);
    geode::Mod::setFallbackMod(modB);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dirA.path);

    REQUIRE(luax::currentMod() == modA);
    REQUIRE(geode::Mod::get() != modA);
}

TEST_CASE("currentMod returns nullptr when no mod matches resources root") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_current_mod_"};

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);

    REQUIRE(luax::currentMod() == nullptr);
}

TEST_CASE("currentMod rejects stale cached mod after unregister") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_current_mod_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    REQUIRE(luax::currentMod() == mod);

    geode::Mod::destroy(mod);
    REQUIRE(luax::currentMod() == nullptr);
}

TEST_CASE("invalidateCurrentModCache drops cached lookup") {
    RuntimeGuard guard;
    luauapi_test::ScopedTempDir dir{"luauapi_current_mod_"};
    auto* mod = geode::Mod::create(dir.path);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir.path);
    REQUIRE(luax::currentMod() == mod);

    geode::Mod::destroy(mod);
    auto* replacement = geode::Mod::create(dir.path);
    luax::invalidateCurrentModCache();

    REQUIRE(luax::currentMod() == replacement);
}
