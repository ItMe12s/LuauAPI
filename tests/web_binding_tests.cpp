#include "bindings/geode/CurrentMod.hpp"
#include "bindings/geode/web/WebInternal.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "host/lua_test_helpers.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/utils/web.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <lua.h>
#include <lualib.h>
#include <optional>
#include <string>
#include <thread>

namespace luax {
    geode::Result<void> registerGeodeWeb(lua_State* L);
} // namespace luax

namespace {
    using namespace luax;

    int testSizedString(lua_State* L) {
        auto size = static_cast<std::size_t>(luaL_checkinteger(L, 1));
        std::string value(size, 'x');
        lua_pushlstring(L, value.data(), value.size());
        return 1;
    }

    struct WebBindingGuard {
        WebBindingGuard() {
            Runtime::setMainThreadId(std::this_thread::get_id());
            resetBindingsForTests();
        }

        ~WebBindingGuard() {
            geode::utils::web::test::resetResponseFactory();
            clearWebState();
            invalidateCurrentModCache();
            geode::Mod::resetForTests();
            Runtime::resetForTests();
            resetBindingsForTests();
        }
    };

    struct ModFixture {
        std::filesystem::path dir;
        geode::Mod* mod = nullptr;
        lua_State* L = nullptr;

        ModFixture() {
            dir = std::filesystem::temp_directory_path() /
                ("luauapi_web_binding_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            REQUIRE(std::filesystem::create_directories(dir));
            mod = geode::Mod::create(dir);

            auto* runtime = Runtime::getOrCreate();
            runtime->setResourcesRoot(dir);
            L = runtime->state();
            registerWebBindings(L);
        }

        ~ModFixture() {
            if (std::filesystem::exists(dir)) {
                std::filesystem::remove_all(dir);
            }
            if (mod) {
                geode::Mod::destroy(mod);
            }
        }

        void registerWebBindings(lua_State* state) {
            registerBinding({"geode_web", &registerGeodeWeb, 0});
            REQUIRE(applyAllBindings(state) == std::nullopt);
            lua_pushcfunction(state, testSizedString, "testSizedString");
            lua_setglobal(state, "__test_sized_string");
        }
    };

    bool runScriptReturnsBool(lua_State* L, std::string const& source) {
        luauapi_test::loadFunction(L, source, "=web_binding_test");
        if (lua_pcall(L, 0, 1, 0) != 0) {
            if (lua_isstring(L, -1)) {
                INFO(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
            return false;
        }
        if (!lua_isboolean(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        bool const value = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return value;
    }

    std::optional<std::string> runScriptReturnsString(lua_State* L, std::string const& source) {
        luauapi_test::loadFunction(L, source, "=web_binding_test");
        if (lua_pcall(L, 0, 1, 0) != 0) {
            if (lua_isstring(L, -1)) {
                return std::string(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
            return std::nullopt;
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        size_t len = 0;
        char const* text = lua_tolstring(L, -1, &len);
        std::string value(text ? text : "", len);
        lua_pop(L, 1);
        return value;
    }

    void writeFileContents(std::filesystem::path const& path, std::string const& data) {
        std::error_code ec;
        auto parent = path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
        }
        std::ofstream out(path, std::ios::binary);
        REQUIRE(out.good());
        out << data;
    }

    void setOversizedResponseFactory() {
        geode::utils::web::test::setResponseFactory(
            [](geode::utils::web::WebRequest const&, std::string_view, std::string_view, geode::Mod*) {
                geode::utils::web::ByteVector body(kMaxWebResponseBytes + 1, 'x');
                return geode::utils::web::test::makeResponseWithBody(std::move(body));
            }
        );
    }

    std::string oversizedStringCall(std::size_t size) {
        return "__test_sized_string(" + std::to_string(size) + ")";
    }
} // namespace

TEST_CASE("geode.utils.web newRequest and multipart metatables smoke") {
    WebBindingGuard guard;
    ModFixture fixture;

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        R"(
        local req = geode.utils.web.newRequest()
        if type(req) ~= "userdata" then
            return false
        end
        if type(req.header) ~= "function" then
            return false
        end

        local form = geode.utils.web.multipart()
        if type(form) ~= "userdata" then
            return false
        end
        if type(form.param) ~= "function" then
            return false
        end
        return form:param("name", "value") == form
    )"
    ));
}

TEST_CASE("geode.utils.web mock get completes on main thread") {
    WebBindingGuard guard;
    ModFixture fixture;

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        R"(
        local ok = false
        geode.utils.web.get("http://example.test", function(res, err)
            ok = res ~= nil and err == nil and res:ok() and res:text() == "OK"
        end)
        return ok
    )"
    ));
}

TEST_CASE("WebRequest bodyString rejects oversized payload") {
    WebBindingGuard guard;
    ModFixture fixture;

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        std::string(R"(
        local req = geode.utils.web.newRequest()
        local body = )") +
            oversizedStringCall(kMaxWebRequestBytes + 1) +
            R"(
        local result, err = req:bodyString(body)
        return result == nil and err == "request body exceeds maximum size"
    )"
    ));
}

TEST_CASE("WebRequest bodyJson rejects oversized payload") {
    WebBindingGuard guard;
    ModFixture fixture;

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        std::string(R"(
        local req = geode.utils.web.newRequest()
        local payload = { data = )") +
            oversizedStringCall(kMaxWebRequestBytes + 1) +
            R"( }
        local result, err = req:bodyJson(payload)
        return result == nil and err == "request body exceeds maximum size"
    )"
    ));
}

TEST_CASE("MultipartForm rejects cumulative body overflow") {
    WebBindingGuard guard;
    ModFixture fixture;

    auto const firstSize = kMaxWebRequestBytes - 100;
    auto const secondSize = 200;

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        std::string(R"(
        local form = geode.utils.web.multipart()
        local first = )") +
            oversizedStringCall(firstSize) +
            R"(
        local second = )" +
            oversizedStringCall(secondSize) +
            R"(
        if form:param("first", first) ~= form then
            return false
        end
        local result, err = form:param("second", second)
        return result == nil and err == "request body exceeds maximum size"
    )"
    ));
}

TEST_CASE("geode.utils.web get callback rejects oversized response") {
    WebBindingGuard guard;
    ModFixture fixture;
    setOversizedResponseFactory();

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        R"(
        local callbackErr = nil
        geode.utils.web.get("http://example.test", function(res, err)
            callbackErr = err
            return res == nil and err == "response exceeds maximum size"
        end)
        return callbackErr == "response exceeds maximum size"
    )"
    ));
}

TEST_CASE("WebResponse saveTo writes inside sandbox root") {
    WebBindingGuard guard;
    ModFixture fixture;
    REQUIRE(std::filesystem::create_directories(fixture.mod->getSaveDir()));

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        R"(
        local saved = false
        geode.utils.web.get("http://example.test", function(res, err)
            if not res then
                return
            end
            local ok, saveErr = res:saveTo("save", "nested/out.txt")
            saved = ok == true and saveErr == nil
        end)
        return saved
    )"
    ));

    auto const output = fixture.mod->getSaveDir() / "nested" / "out.txt";
    REQUIRE(std::filesystem::exists(output));
    std::ifstream in(output, std::ios::binary);
    REQUIRE(in.good());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    REQUIRE(contents == "OK");
}

TEST_CASE("WebResponse saveTo rejects sandbox escape") {
    WebBindingGuard guard;
    ModFixture fixture;
    REQUIRE(std::filesystem::create_directories(fixture.mod->getSaveDir()));

    auto err = runScriptReturnsString(
        fixture.L,
        R"(
        local saveErr = nil
        geode.utils.web.get("http://example.test", function(res, err)
            if not res then
                return
            end
            local ok, errMsg = res:saveTo("save", "../escape.txt")
            saveErr = errMsg
        end)
        return saveErr
    )"
    );
    REQUIRE(err.has_value());
    REQUIRE(err->find("escapes") != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(fixture.dir / "escape.txt"));
}

TEST_CASE("WebResponse saveTo rejects oversized body") {
    WebBindingGuard guard;
    ModFixture fixture;
    setOversizedResponseFactory();
    REQUIRE(std::filesystem::create_directories(fixture.mod->getSaveDir()));

    auto err = runScriptReturnsString(
        fixture.L,
        R"(
        local saveErr = nil
        geode.utils.web.get("http://example.test", function(res, err)
            if res then
                saveErr = "unexpected response"
                return
            end
            saveErr = err
        end)
        return saveErr
    )"
    );
    REQUIRE(err.has_value());
    REQUIRE(*err == "response exceeds maximum size");
}

TEST_CASE("MultipartForm fileFrom rejects sandbox escape") {
    WebBindingGuard guard;
    ModFixture fixture;
    REQUIRE(std::filesystem::create_directories(fixture.mod->getSaveDir()));
    writeFileContents(fixture.mod->getSaveDir() / "payload.bin", "payload");

    auto err = runScriptReturnsString(
        fixture.L,
        R"(
        local form = geode.utils.web.multipart()
        local result, errMsg = form:fileFrom("field", "save", "../payload.bin")
        return errMsg
    )"
    );
    REQUIRE(err.has_value());
    REQUIRE(err->find("escapes") != std::string::npos);
}

TEST_CASE("geode.utils.web get without current mod errors before send") {
    WebBindingGuard guard;
    auto dir = std::filesystem::temp_directory_path() /
        ("luauapi_web_no_mod_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    REQUIRE(std::filesystem::create_directories(dir));

    auto* runtime = Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    registerBinding({"geode_web", &registerGeodeWeb, 0});
    REQUIRE(applyAllBindings(L) == std::nullopt);

    auto err = runScriptReturnsString(
        L,
        R"(
        local ok, pcallErr = pcall(function()
            geode.utils.web.get("http://example.test", function() end)
        end)
        if ok then
            return "expected pcall failure"
        end
        return pcallErr
    )"
    );
    REQUIRE(err.has_value());
    REQUIRE(err->find("current mod is unavailable") != std::string::npos);

    std::filesystem::remove_all(dir);
}

TEST_CASE("geode.utils.web fetch without current mod errors before send") {
    WebBindingGuard guard;
    auto dir = std::filesystem::temp_directory_path() /
        ("luauapi_web_fetch_no_mod_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    REQUIRE(std::filesystem::create_directories(dir));

    auto* runtime = Runtime::getOrCreate();
    runtime->setResourcesRoot(dir);
    auto* L = runtime->state();
    registerBinding({"geode_web", &registerGeodeWeb, 0});
    REQUIRE(applyAllBindings(L) == std::nullopt);

    auto err = runScriptReturnsString(
        L,
        R"(
        local ok, pcallErr = pcall(function()
            geode.utils.web.fetch("http://example.test", {}, function() end)
        end)
        if ok then
            return "expected pcall failure"
        end
        return pcallErr
    )"
    );
    REQUIRE(err.has_value());
    REQUIRE(err->find("current mod is unavailable") != std::string::npos);

    std::filesystem::remove_all(dir);
}

TEST_CASE("geode.utils.web.openLinkInBrowser validates url argument") {
    WebBindingGuard guard;
    ModFixture fixture;

    REQUIRE(runScriptReturnsBool(
        fixture.L,
        R"(
        local ok = pcall(function()
            geode.utils.web.openLinkInBrowser(nil)
        end)
        return ok == false
    )"
    ));
}
