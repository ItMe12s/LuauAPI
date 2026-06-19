#include "bindings/geode/CurrentMod.hpp"
#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "bindings/imgui/ImGuiFontRegistry.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "host/ImGuiTestHarness.hpp"
#include "host/lua_test_helpers.hpp"

#include <Geode/loader/Mod.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <imgui.h>
#include <initializer_list>
#include <lua.h>
#include <lualib.h>
#include <string_view>
#include <thread>

namespace luax {
    geode::Result<void> registerImGui(lua_State* L);
}

namespace {
    using Catch::Approx;

    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::resetBindingsForTests();
        }

        ~RuntimeGuard() {
            luax::ImGuiDrawScheduler::get().clear();
            luax::imguiFontClear();
            luax::invalidateCurrentModCache();
            geode::Mod::resetForTests();
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };

    struct ImGuiContextGuard : luauapi_test::ImGuiTestContext {};

    void registerImGuiBinding(lua_State* L) {
        luax::registerBinding({"imgui_lib", &luax::registerImGui, 10});
        REQUIRE(luax::applyAllBindings(L) == std::nullopt);
    }

    int readImGuiIntField(lua_State* L, std::initializer_list<char const*> path) {
        lua_getglobal(L, "imgui");
        REQUIRE(lua_istable(L, -1));
        for (char const* key : path) {
            lua_getfield(L, -1, key);
            lua_remove(L, -2);
        }
        REQUIRE(lua_isnumber(L, -1));
        int const value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    void runImGuiDraw(lua_State* L, std::string_view source) {
        auto ref = luauapi_test::makeCallback(L, source);
        auto& scheduler = luax::ImGuiDrawScheduler::get();
        auto const id = scheduler.add(std::move(ref));
        REQUIRE(id != 0);
        luauapi_test::beginImGuiTestFrame();
        scheduler.drawAll();
        luauapi_test::endImGuiTestFrame();
    }

    bool globalIsTrue(lua_State* L, char const* name) {
        lua_getglobal(L, name);
        bool const value = lua_isboolean(L, -1) && lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return value;
    }

    bool callImGuiTextOutsideDraw(lua_State* L) {
        lua_getglobal(L, "imgui");
        if (!lua_istable(L, -1)) {
            lua_settop(L, 0);
            return false;
        }
        lua_getfield(L, -1, "text");
        lua_remove(L, -2);
        lua_pushstring(L, "outside frame");
        int const status = lua_pcall(L, 1, 0, 0);
        if (status != 0) {
            lua_pop(L, 1);
            return true;
        }
        return false;
    }

    bool hasGlobalField(lua_State* L, std::initializer_list<char const*> path) {
        lua_getglobal(L, path.begin()[0]);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        auto it = path.begin();
        ++it;
        for (; it != path.end(); ++it) {
            lua_getfield(L, -1, *it);
            lua_remove(L, -2);
        }
        bool const ok = !lua_isnil(L, -1);
        lua_pop(L, 1);
        return ok;
    }

    bool callFontAddBadSize(lua_State* L) {
        lua_getglobal(L, "imgui");
        lua_getfield(L, -1, "font");
        lua_getfield(L, -1, "add");
        lua_pushstring(L, "resources");
        lua_pushstring(L, "assets/test.ttf");
        lua_pushnumber(L, 0);
        int const status = lua_pcall(L, 3, 0, 0);
        if (status != 0) {
            lua_settop(L, 0);
            return true;
        }
        lua_settop(L, 0);
        return false;
    }

    bool callFontWithOutsideDraw(lua_State* L) {
        lua_settop(L, 0);
        lua_getglobal(L, "imgui");
        lua_getfield(L, -1, "font");
        lua_getfield(L, -1, "with");
        lua_pushnil(L);
        lua_pushcfunction(
            L,
            [](lua_State*) -> int {
                return 0;
            },
            "noop"
        );
        int const status = lua_pcall(L, 2, 0, 0);
        if (status != 0) {
            lua_settop(L, 0);
            return true;
        }
        lua_settop(L, 0);
        return false;
    }

    struct TestMod {
        std::filesystem::path dir;
        geode::Mod* mod = nullptr;

        TestMod() {
            dir = std::filesystem::temp_directory_path() /
                ("luauapi_imgui_font_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            REQUIRE(std::filesystem::create_directories(dir));
            mod = geode::Mod::create(dir);
        }

        ~TestMod() {
            if (mod) {
                geode::Mod::destroy(mod);
            }
            std::error_code ec;
            std::filesystem::remove_all(dir, ec);
        }
    };

    void bindModResources(luax::Runtime* runtime, geode::Mod* mod) {
        runtime->setResourcesRoot(mod->getResourcesDir());
    }
} // namespace

TEST_CASE("ImGui constants match Dear ImGui values") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    REQUIRE(readImGuiIntField(L, {"Flag", "Window", "NoTitleBar"}) == ImGuiWindowFlags_NoTitleBar);
    REQUIRE(readImGuiIntField(L, {"Col", "Text"}) == ImGuiCol_Text);
    REQUIRE(readImGuiIntField(L, {"StyleVar", "Alpha"}) == ImGuiStyleVar_Alpha);
    REQUIRE(readImGuiIntField(L, {"Cond", "FirstUseEver"}) == ImGuiCond_FirstUseEver);
}

TEST_CASE("ImGui widgets reject calls outside onDraw") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    REQUIRE(callImGuiTextOutsideDraw(L));
}

TEST_CASE("ImGui host visibility stubs stay safe in host tests") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    runImGuiDraw(L, R"(
        imgui.setVisible(true)
        imgui.toggle()
        _G.hostVisible = imgui.isVisible()
    )");

    REQUIRE_FALSE(globalIsTrue(L, "hostVisible"));
}

TEST_CASE("imgui.style.with restores pushed style after closure") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    float const baseAlpha = ImGui::GetStyle().Alpha;

    runImGuiDraw(L, R"(
        imgui.style.with({ alpha = 0.25 }, function()
        end)
    )");

    REQUIRE(ImGui::GetStyle().Alpha == Approx(baseAlpha));
}

TEST_CASE("imgui.style.with restores pushed style after closure error") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    float const baseAlpha = ImGui::GetStyle().Alpha;

    runImGuiDraw(L, R"(
        imgui.style.with({ alpha = 0.25 }, function()
            error("style.with failed")
        end)
    )");

    REQUIRE(ImGui::GetStyle().Alpha == Approx(baseAlpha));
}

TEST_CASE("imgui.window closes after closure error and allows another window") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    runImGuiDraw(L, R"(
        imgui.window("First", function()
            error("window failed")
        end)
        imgui.window("Second", function()
            _G.secondWindowOk = true
        end)
    )");

    REQUIRE(globalIsTrue(L, "secondWindowOk"));
}

TEST_CASE("imgui.tabBar closes after closure error and allows another tab bar") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    runImGuiDraw(L, R"(
        imgui.window("Tabs", function()
            imgui.tabBar("Broken", function()
                error("tabBar failed")
            end)
            imgui.tabBar("Recovered", function()
                imgui.tabItem("One", function()
                    _G.recoveredTabOk = true
                end)
            end)
        end)
    )");

    REQUIRE(globalIsTrue(L, "recoveredTabOk"));
}

TEST_CASE("imgui.group closes after closure error and allows another group") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    runImGuiDraw(L, R"(
        imgui.window("Groups", function()
            imgui.group(function()
                error("group failed")
            end)
            imgui.group(function()
                _G.recoveredGroupOk = true
            end)
        end)
    )");

    REQUIRE(globalIsTrue(L, "recoveredGroupOk"));
}

TEST_CASE("imgui.child closes after closure error and allows another child") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    runImGuiDraw(L, R"(
        imgui.window("Children", function()
            imgui.child("Broken", function()
                error("child failed")
            end)
            imgui.child("Recovered", function()
                _G.recoveredChildOk = true
            end)
        end)
    )");

    REQUIRE(globalIsTrue(L, "recoveredChildOk"));
}

TEST_CASE("imgui.font namespace is registered") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    REQUIRE(hasGlobalField(L, {"imgui", "font", "add"}));
    REQUIRE(hasGlobalField(L, {"imgui", "font", "with"}));
}

TEST_CASE("imgui.font.add rejects non-positive size") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    TestMod testMod;
    auto* runtime = luax::Runtime::getOrCreate();
    bindModResources(runtime, testMod.mod);
    auto* L = runtime->state();
    registerImGuiBinding(L);

    REQUIRE(callFontAddBadSize(L));
}

TEST_CASE("imgui.font.with rejects calls outside onDraw") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    registerImGuiBinding(L);

    REQUIRE(callFontWithOutsideDraw(L));
}

TEST_CASE("imgui.font.add rejects calls inside onDraw") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    TestMod testMod;
    auto* runtime = luax::Runtime::getOrCreate();
    bindModResources(runtime, testMod.mod);
    auto* L = runtime->state();
    registerImGuiBinding(L);

    runImGuiDraw(L, R"(
        local ok, err = pcall(function()
            imgui.font.add("resources", "assets/test.ttf", 16)
        end)
        _G.fontAddInsideDrawFailed = not ok
    )");

    REQUIRE(globalIsTrue(L, "fontAddInsideDrawFailed"));
}

#ifdef LUAUAPI_TEST_FONT_PATH
TEST_CASE("imgui.font loads resource fonts and font.with pops after closure error") {
    RuntimeGuard guard;
    ImGuiContextGuard ctx;
    TestMod testMod;
    auto assets = testMod.dir / "assets";
    REQUIRE(std::filesystem::create_directories(assets));
    std::filesystem::copy_file(
        LUAUAPI_TEST_FONT_PATH, assets / "DroidSans.ttf", std::filesystem::copy_options::overwrite_existing
    );

    auto* runtime = luax::Runtime::getOrCreate();
    bindModResources(runtime, testMod.mod);
    auto* L = runtime->state();
    registerImGuiBinding(L);

    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(
        _G.testFont = imgui.font.add("resources", "assets/DroidSans.ttf", 16)
        return _G.testFont ~= nil
    )"));

    runImGuiDraw(L, R"(
        imgui.window("Fonts", function()
            imgui.font.with(_G.testFont, function()
                error("font.with failed")
            end)
            imgui.font.with(_G.testFont, function()
                _G.recoveredFontWithOk = true
            end)
        end)
    )");

    REQUIRE(globalIsTrue(L, "recoveredFontWithOk"));
}
#endif
