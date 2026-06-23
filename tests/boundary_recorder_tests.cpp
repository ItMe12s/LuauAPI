#include "bindings/geode/CurrentMod.hpp"
#include "core/Runtime.hpp"
#include "core/StackFormat.hpp"
#include "diagnostics/BoundaryRecorder.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Dirs.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/log.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <lua.h>
#include <lua_test_helpers.hpp>
#include <lualib.h>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

namespace {
    struct SidecarGuard {
        luauapi_test::RuntimeGuard runtime;
        luauapi_test::ScopedTempDir dir;

        SidecarGuard() {
            luax::diag::resetForTests();
            luax::diag::setRecordingEnabled(true);
            luax::diag::setCrashlogsDirForTests(dir.path);
        }

        ~SidecarGuard() {
            luax::diag::setCrashlogsDirForTests(std::filesystem::path{});
            luax::diag::resetForTests();
        }
    };

    std::optional<std::string> readSidecar(std::filesystem::path const& dir) {
        auto path = dir / luax::kSidecarFileName;
        std::ifstream in(path, std::ios::binary);
        if (!in.good()) return std::nullopt;
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
} // namespace

TEST_CASE("BoundaryRecorder records and exposes the active boundary", "[diagnostics][sidecar]") {
    SidecarGuard guard;

    luax::diag::BoundaryFrame f;
    f.kind = luax::diag::BoundaryKind::HookBefore;
    f.target = "PlayLayer.update";
    f.callbackId = 42;
    auto scope = luax::diag::pushBoundary(std::move(f), nullptr);

    auto const* active = luax::diag::activeBoundary();
    REQUIRE(active != nullptr);
    REQUIRE(active->kind == luax::diag::BoundaryKind::HookBefore);
    REQUIRE(active->target == "PlayLayer.update");
    REQUIRE(active->callbackId == 42);
    REQUIRE_FALSE(active->timestamp.empty());
}

TEST_CASE("BoundaryScope pop restores the outer active boundary", "[diagnostics][sidecar]") {
    SidecarGuard guard;

    luax::diag::BoundaryFrame outer;
    outer.kind = luax::diag::BoundaryKind::Script;
    outer.target = "outer";
    auto outerScope = luax::diag::pushBoundary(std::move(outer), nullptr);

    luax::diag::BoundaryFrame inner;
    inner.kind = luax::diag::BoundaryKind::HookBefore;
    inner.target = "inner";
    {
        auto innerScope = luax::diag::pushBoundary(std::move(inner), nullptr);
        REQUIRE(luax::diag::activeBoundary()->target == "inner");
    }

    auto const* active = luax::diag::activeBoundary();
    REQUIRE(active != nullptr);
    REQUIRE(active->target == "outer");
}

TEST_CASE("BoundaryRecorder retains last snapshot when stack pops to empty", "[diagnostics][sidecar]") {
    SidecarGuard guard;

    luax::diag::BoundaryFrame f;
    f.kind = luax::diag::BoundaryKind::HookAfter;
    f.target = "geode.gd.MenuLayer:init/0";
    f.callbackId = 1;
    {
        auto scope = luax::diag::pushBoundary(std::move(f), nullptr);
        luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready, true);
    }

    auto contents = readSidecar(guard.dir.path);
    REQUIRE(contents.has_value());
    CHECK(contents->find("target: geode.gd.MenuLayer:init/0") != std::string::npos);
    CHECK(contents->find("=== Active boundary (innermost) ===\n(none)") == std::string::npos);
}

TEST_CASE("BoundaryRecorder serializes call chain innermost first", "[diagnostics][sidecar]") {
    SidecarGuard guard;

    luax::diag::BoundaryFrame outer;
    outer.kind = luax::diag::BoundaryKind::Script;
    outer.target = "outer";
    auto outerScope = luax::diag::pushBoundary(std::move(outer), nullptr);

    luax::diag::BoundaryFrame inner;
    inner.kind = luax::diag::BoundaryKind::HookAfter;
    inner.target = "inner";
    inner.callbackId = 3;
    auto innerScope = luax::diag::pushBoundary(std::move(inner), nullptr);

    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready, true);
    auto contents = readSidecar(guard.dir.path);
    REQUIRE(contents.has_value());
    CHECK(contents->find("=== Call chain (innermost first) ===") != std::string::npos);
    CHECK(contents->find("#0 hook-after  inner") != std::string::npos);
    CHECK(contents->find("#1 script  outer") != std::string::npos);
}

TEST_CASE("BoundaryRecorder captures the Lua stack at binding entry", "[diagnostics][sidecar]") {
    SidecarGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    static std::string capturedStack;
    auto captureFromC = [](lua_State* state) -> int {
        auto boundary = luax::diag::recordBindingEntry(
            state, "nativefn", luax::diag::BoundaryKind::GeneratedBinding
        );
        (void)boundary;
        if (auto const* active = luax::diag::activeBoundary()) {
            capturedStack = active->luaStack;
        }
        return 0;
    };

    lua_pushcfunction(L, captureFromC, "nativefn");
    lua_setglobal(L, "nativefn");

    capturedStack.clear();
    REQUIRE(runtime->runScript("nativefn()", "@scripts/stacktest.luau").isOk());

    REQUIRE_FALSE(capturedStack.empty());
    CHECK(luax::hasStackFrames(capturedStack));
    CHECK(capturedStack.find("stacktest") != std::string::npos);
}

TEST_CASE("runScript captures pending call source in boundary stack", "[diagnostics][sidecar]") {
    SidecarGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    static std::string capturedStack;
    auto captureFromC = [](lua_State* state) -> int {
        if (auto const* active = luax::diag::activeBoundary()) {
            capturedStack = active->luaStack;
        }
        return 0;
    };

    auto* L = runtime->state();
    lua_pushcfunction(L, captureFromC, "capture");
    lua_setglobal(L, "capture");

    capturedStack.clear();
    REQUIRE(runtime->runScript("capture()\nprint(\"ok\")", "@scripts/pendingstack.luau").isOk());

    CHECK(luax::hasStackFrames(capturedStack));
    CHECK(capturedStack.find("pendingstack") != std::string::npos);
}

TEST_CASE(
    "BoundaryRecorder flush writes the sidecar file with the expected shape",
    "[diagnostics][sidecar]"
) {
    SidecarGuard guard;

    luax::diag::BoundaryFrame f;
    f.kind = luax::diag::BoundaryKind::HookTrampoline;
    f.target = "PlayLayer.update";
    f.callbackId = 7;
    auto scope = luax::diag::pushBoundary(std::move(f), nullptr);

    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready);

    auto contents = readSidecar(guard.dir.path);
    REQUIRE(contents.has_value());

    auto const& text = *contents;
    CHECK(text.find("=== LuauAPI Context ===") != std::string::npos);
    CHECK(text.find("runtime_status: Ready") != std::string::npos);
    CHECK(text.find("=== Active boundary (innermost) ===") != std::string::npos);
    CHECK(text.find("kind: hook-trampoline") != std::string::npos);
    CHECK(text.find("target: PlayLayer.update") != std::string::npos);
    CHECK(text.find("=== Call chain (innermost first) ===") != std::string::npos);
    CHECK(text.find("hook-trampoline  PlayLayer.update") != std::string::npos);
    CHECK(text.find("=== Notes ===") != std::string::npos);
}

TEST_CASE("BoundaryRecorder flush is atomic: no .tmp left behind", "[diagnostics][sidecar]") {
    SidecarGuard guard;

    luax::diag::BoundaryFrame f;
    f.kind = luax::diag::BoundaryKind::Task;
    f.target = "task.spawn";
    auto scope = luax::diag::pushBoundary(std::move(f), nullptr);
    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready);

    CHECK(std::filesystem::exists(guard.dir.path / luax::kSidecarFileName));
    CHECK_FALSE(std::filesystem::exists(guard.dir.path / luax::kSidecarTempName));
}

TEST_CASE("BoundaryRecorder change-or-interval throttle", "[diagnostics][sidecar]") {
    SidecarGuard guard;

    luax::diag::BoundaryFrame f;
    f.kind = luax::diag::BoundaryKind::Script;
    f.target = "t1";
    auto scope1 = luax::diag::pushBoundary(std::move(f), nullptr);
    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready);
    auto first = readSidecar(guard.dir.path);
    REQUIRE(first.has_value());
    CHECK(first->find("target: t1") != std::string::npos);

    luax::diag::BoundaryFrame f2;
    f2.kind = luax::diag::BoundaryKind::Script;
    f2.target = "t1";
    auto scope2 = luax::diag::pushBoundary(std::move(f2), nullptr);
    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready);

    luax::diag::BoundaryFrame f3;
    f3.kind = luax::diag::BoundaryKind::Script;
    f3.target = "t2";
    auto scope3 = luax::diag::pushBoundary(std::move(f3), nullptr);
    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready);
    auto contents = readSidecar(guard.dir.path);
    REQUIRE(contents.has_value());
    CHECK(contents->find("target: t2") != std::string::npos);

    std::this_thread::sleep_for(std::chrono::milliseconds(luax::kSidecarFlushIntervalMs + 20));
    luax::diag::BoundaryFrame f4;
    f4.kind = luax::diag::BoundaryKind::Script;
    f4.target = "t2";
    auto scope4 = luax::diag::pushBoundary(std::move(f4), nullptr);
    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready);
    auto after = readSidecar(guard.dir.path);
    REQUIRE(after.has_value());
}

TEST_CASE("BoundaryRecorder empty mod context still writes blank fields", "[diagnostics][sidecar]") {
    SidecarGuard guard;
    luax::diag::BoundaryFrame f;
    f.kind = luax::diag::BoundaryKind::Script;
    f.target = "orphan";
    auto scope = luax::diag::pushBoundary(std::move(f), nullptr);

    auto const* active = luax::diag::activeBoundary();
    REQUIRE(active != nullptr);
    CHECK(active->modId.empty());
    CHECK(active->modVersion.empty());

    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready);
    auto contents = readSidecar(guard.dir.path);
    REQUIRE(contents.has_value());
    CHECK(contents->find("mod_id: (none)") != std::string::npos);
}

TEST_CASE("BoundaryRecorder I/O failure is swallowed, no throw", "[diagnostics][sidecar]") {
    SidecarGuard guard;
    auto badPath = guard.dir.path / "i_am_a_file";
    {
        std::ofstream touch(badPath);
        REQUIRE(touch.good());
    }
    luax::diag::setCrashlogsDirForTests(badPath / "crashlogs");

    luax::diag::BoundaryFrame f;
    f.kind = luax::diag::BoundaryKind::Script;
    f.target = "bad";
    auto scope = luax::diag::pushBoundary(std::move(f), nullptr);
    REQUIRE_NOTHROW(luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready));
    CHECK_FALSE(std::filesystem::exists(badPath / "crashlogs" / luax::kSidecarFileName));
}

TEST_CASE("BoundaryRecorder disabled gate skips all work", "[diagnostics][sidecar]") {
    SidecarGuard guard;
    luax::diag::setRecordingEnabled(false);

    luax::diag::BoundaryFrame f;
    f.kind = luax::diag::BoundaryKind::Script;
    f.target = "skipped";
    auto scope = luax::diag::pushBoundary(std::move(f), nullptr);

    CHECK(luax::diag::activeBoundary() == nullptr);
    luax::diag::flushIfNeeded(imes::luauapi::RuntimeStatus::Ready);
    CHECK_FALSE(std::filesystem::exists(guard.dir.path / luax::kSidecarFileName));
}

TEST_CASE("Runtime print/warn log with [mod.id] prefix when a mod is in scope", "[runtime][logging]") {
    luauapi_test::ModRuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    luauapi_test::ScopedTempDir modRoot;
    std::filesystem::create_directories(modRoot.path);
    auto* mod = geode::Mod::create(modRoot.path, "cool-level-utils", "v1.2.0");
    runtime->setResourcesRoot(modRoot.path);

    geode::log::clearCapturedMessages();

    REQUIRE(runtime->runScript("print(\"hi, im printed\")", "@scripts/main.luau").isOk());

    bool sawPrefixed = false;
    for (auto const& msg : geode::log::capturedInfoMessages()) {
        if (msg.find("[cool-level-utils] hi, im printed") != std::string::npos) {
            sawPrefixed = true;
            break;
        }
    }
    CHECK(sawPrefixed);
}

TEST_CASE("Runtime print/warn falls back to [lua] when no mod is in scope", "[runtime][logging]") {
    luauapi_test::RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    geode::log::clearCapturedMessages();
    REQUIRE(runtime->runScript("print(\"hi, im printed\")", "@scripts/main.luau").isOk());

    bool sawFallback = false;
    for (auto const& msg : geode::log::capturedInfoMessages()) {
        if (msg.find("[lua] hi, im printed") != std::string::npos) {
            sawFallback = true;
            break;
        }
    }
    CHECK(sawFallback);
}
