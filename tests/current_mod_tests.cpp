#include "lua/bindings/geode/CurrentMod.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/loader/Mod.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <thread>

namespace {
    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::invalidateCurrentModCache();
            geode::Mod::resetForTests();
            luax::Runtime::resetForTests();
        }
    };

    std::filesystem::path makeTempDir() {
        auto dir = std::filesystem::temp_directory_path() /
            ("luauapi_current_mod_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        REQUIRE(std::filesystem::create_directories(dir));
        return dir;
    }
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
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);

    REQUIRE(luax::currentMod() == mod);

    std::filesystem::remove_all(dir);
}

TEST_CASE("currentMod does not fall back to Mod::get") {
    RuntimeGuard guard;
    auto dirA = makeTempDir();
    auto dirB = makeTempDir();
    auto* modA = geode::Mod::create(dirA);
    auto* modB = geode::Mod::create(dirB);
    geode::Mod::setFallbackMod(modB);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dirA);

    REQUIRE(luax::currentMod() == modA);
    REQUIRE(geode::Mod::get() != modA);

    std::filesystem::remove_all(dirA);
    std::filesystem::remove_all(dirB);
}

TEST_CASE("currentMod returns nullptr when no mod matches resources root") {
    RuntimeGuard guard;
    auto dir = makeTempDir();

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);

    REQUIRE(luax::currentMod() == nullptr);

    std::filesystem::remove_all(dir);
}

TEST_CASE("currentMod rejects stale cached mod after unregister") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    REQUIRE(luax::currentMod() == mod);

    geode::Mod::destroy(mod);
    REQUIRE(luax::currentMod() == nullptr);

    std::filesystem::remove_all(dir);
}

TEST_CASE("invalidateCurrentModCache drops cached lookup") {
    RuntimeGuard guard;
    auto dir = makeTempDir();
    auto* mod = geode::Mod::create(dir);

    auto* runtime = luax::Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    REQUIRE(luax::currentMod() == mod);

    geode::Mod::destroy(mod);
    auto* replacement = geode::Mod::create(dir);
    luax::invalidateCurrentModCache();

    REQUIRE(luax::currentMod() == replacement);

    std::filesystem::remove_all(dir);
}
