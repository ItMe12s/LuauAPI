#include "bindings/geode/ScriptEventInternal.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "host/lua_test_helpers.hpp"

#include <Geode/loader/Event.hpp>
#include <ScriptEvents.hpp>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <string>
#include <string_view>
#include <thread>

namespace luax {
    geode::Result<void> registerGeodeScriptEvent(lua_State* L);
} // namespace luax

namespace {
    using imes::luauapi::LuaScriptEvent;
    using imes::luauapi::postScriptEvent;
    using luauapi_test::collectGarbage;
    using luauapi_test::globalInteger;
    using luauapi_test::globalIsNil;
    using luauapi_test::runScriptPcall;
    using luauapi_test::runScriptReturnsBool;

    struct ScriptEventTestGuard {
        ScriptEventTestGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            geode::test::bindMainThreadToCurrent();
            luax::resetBindingsForTests();
            luax::registerBinding({"geode_script_event_lib", &luax::registerGeodeScriptEvent, 10});
        }

        ~ScriptEventTestGuard() {
            geode::test::clearMainThreadQueue();
            geode::test::resetEvents();
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }

        lua_State* makeState() {
            auto* runtime = luax::Runtime::getOrCreate();
            REQUIRE(runtime != nullptr);
            auto* L = runtime->state();
            REQUIRE(L != nullptr);
            REQUIRE(luax::applyAllBindings(L) == std::nullopt);
            return L;
        }
    };
} // namespace

TEST_CASE("geode.ScriptEvent.post delivers topic and payload to C++ listeners") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    int calls = 0;
    std::string gotTopic;
    std::string gotPayload;
    auto handle = LuaScriptEvent().listen([&](std::string_view topic, std::string_view payload) {
        ++calls;
        gotTopic = std::string(topic);
        gotPayload = std::string(payload);
        return false;
    });

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.post("sample.topic", "extra data")
    )"));
    REQUIRE(calls == 1);
    REQUIRE(gotTopic == "sample.topic");
    REQUIRE(gotPayload == "extra data");
}

TEST_CASE("geode.ScriptEvent.post defaults payload to empty string") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    int calls = 0;
    std::string gotPayload;
    auto handle = LuaScriptEvent().listen([&](std::string_view, std::string_view payload) {
        ++calls;
        gotPayload = std::string(payload);
        return false;
    });

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.post("no.payload")
    )"));
    REQUIRE(calls == 1);
    REQUIRE(gotPayload.empty());
}

TEST_CASE("geode.ScriptEvent.post does not echo into Lua listeners") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    int cppCalls = 0;
    auto handle = LuaScriptEvent().listen([&](std::string_view, std::string_view) {
        ++cppCalls;
        return false;
    });

    REQUIRE(runScriptReturnsBool(L, R"(
        local echoCalls = 0
        geode.ScriptEvent.listen(function(topic, payload)
            echoCalls = echoCalls + 1
        end)
        geode.ScriptEvent.post("hello", "world")
        return echoCalls == 0
    )"));
    REQUIRE(cppCalls == 1);
}

TEST_CASE("postScriptEvent delivers to Lua listeners on the main thread") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.listen(function(topic, payload)
            _G.seen_topic = topic
            _G.seen_payload = payload
        end)
    )"));

    postScriptEvent("cxx.topic", "cxx.payload");

    REQUIRE(runScriptReturnsBool(L, R"(
        return _G.seen_topic == "cxx.topic" and _G.seen_payload == "cxx.payload"
    )"));
}

TEST_CASE("postScriptEvent also delivers to C++ listeners") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    int cppCalls = 0;
    auto handle = LuaScriptEvent().listen([&](std::string_view, std::string_view) {
        ++cppCalls;
        return false;
    });
    REQUIRE(runScriptPcall(L, R"(
        _G.lua_seen = nil
        geode.ScriptEvent.listen(function()
            _G.lua_seen = true
        end)
    )"));

    postScriptEvent("both.directions", "");

    REQUIRE(cppCalls == 1);
    REQUIRE(runScriptReturnsBool(L, "return _G.lua_seen == true"));
}

TEST_CASE("geode.ScriptEvent.listenFor filters by topic") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.listenFor("alpha", function(topic, payload)
            _G.alpha_seen = payload
        end)
        geode.ScriptEvent.listenFor("beta", function(topic, payload)
            _G.beta_seen = payload
        end)
    )"));

    postScriptEvent("beta", "b1");
    REQUIRE(runScriptReturnsBool(L, R"(
        return _G.alpha_seen == nil and _G.beta_seen == "b1"
    )"));

    postScriptEvent("alpha", "a1");
    REQUIRE(runScriptReturnsBool(L, R"(
        return _G.alpha_seen == "a1" and _G.beta_seen == "b1"
    )"));
}

TEST_CASE("geode.ScriptEvent.listenFor ignores non-matching C++ posts") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        local calls = 0
        geode.ScriptEvent.listenFor("only.this", function()
            calls = calls + 1
        end)
        geode.ScriptEvent.post("only.this", "from lua")
        _G.only_this_calls = calls
    )"));

    postScriptEvent("other.topic", "");
    REQUIRE(runScriptReturnsBool(L, "return _G.only_this_calls == 0"));
}

TEST_CASE("listener handle disconnect stops delivery") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        local handle = geode.ScriptEvent.listen(function()
            _G.disconnect_seen = (_G.disconnect_seen or 0) + 1
        end)
        handle:disconnect()
    )"));

    postScriptEvent("gone.listener", "");
    REQUIRE(runScriptReturnsBool(L, "return _G.disconnect_seen == nil"));
}

TEST_CASE("listener handle garbage collection disconnects") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        local function makeListener()
            geode.ScriptEvent.listen(function()
                _G.gc_seen = (_G.gc_seen or 0) + 1
            end)
        end
        makeListener()
    )"));
    collectGarbage(L);

    postScriptEvent("gc.listener", "");
    REQUIRE(runScriptReturnsBool(L, "return _G.gc_seen == nil"));
}

TEST_CASE("listener priority orders callbacks") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        local order = {}
        geode.ScriptEvent.listen(function()
            order[#order + 1] = "late"
        end, 100)
        geode.ScriptEvent.listen(function()
            order[#order + 1] = "early"
        end, -100)
        _G.seen_order = order
    )"));

    postScriptEvent("ordered", "");
    REQUIRE(runScriptReturnsBool(L, R"(
        return _G.seen_order[1] == "early" and _G.seen_order[2] == "late"
    )"));
}

TEST_CASE("listener returning true stops later listeners") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.listen(function()
            _G.stop_calls = (_G.stop_calls or 0) + 1
            return true
        end)
        geode.ScriptEvent.listen(function()
            _G.stop_calls = (_G.stop_calls or 0) + 1
        end)
    )"));

    postScriptEvent("stopped", "");
    REQUIRE(runScriptReturnsBool(L, "return _G.stop_calls == 1"));
}

TEST_CASE("listener returning true stops listenFor callbacks for the same topic") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.listen(function()
            return true
        end)
        geode.ScriptEvent.listenFor("topic.x", function()
            _G.stopForSeen = true
        end)
    )"));

    postScriptEvent("topic.x", "");
    REQUIRE(globalIsNil(L, "stopForSeen"));
}

TEST_CASE("C++ listener returning true stops Lua listeners") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    auto handle = LuaScriptEvent().listen([](std::string_view, std::string_view) {
        return true;
    });
    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.listen(function()
            _G.cppStopSeen = true
        end)
    )"));

    postScriptEvent("cpp.stop", "");
    REQUIRE(globalIsNil(L, "cppStopSeen"));
}

TEST_CASE("listener error does not stop later listeners") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.listen(function()
            error("boom")
        end)
        geode.ScriptEvent.listen(function()
            _G.errorResilient = (_G.errorResilient or 0) + 1
        end)
    )"));

    postScriptEvent("err", "");
    REQUIRE(globalInteger(L, "errorResilient") == 1);
}

TEST_CASE("geode.ScriptEvent rejects invalid arguments") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE_FALSE(runScriptPcall(L, "geode.ScriptEvent.post({})"));
    REQUIRE_FALSE(runScriptPcall(L, "geode.ScriptEvent.listen('not a function')"));
    REQUIRE_FALSE(runScriptPcall(L, "geode.ScriptEvent.listenFor({}, function() end)"));
}

TEST_CASE("off-main postScriptEvent hops to the main thread") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.listen(function(topic, payload)
            _G.threaded_topic = topic
            _G.threaded_payload = payload
        end)
    )"));

    std::thread worker([] {
        postScriptEvent("thread.topic", "thread.payload");
    });
    worker.join();

    REQUIRE(runScriptReturnsBool(L, "return _G.threaded_topic == nil"));

    geode::test::drainMainThreadQueue();

    REQUIRE(runScriptReturnsBool(L, R"(
        return _G.threaded_topic == "thread.topic" and _G.threaded_payload == "thread.payload"
    )"));
}

TEST_CASE("geode.ScriptEvent namespace shape") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptReturnsBool(L, R"(
        local se = geode.ScriptEvent
        if type(se) ~= "table" then return false end
        if type(se.post) ~= "function" then return false end
        if type(se.listen) ~= "function" then return false end
        if type(se.listenFor) ~= "function" then return false end
        local handle = se.listen(function() end)
        if type(handle) ~= "userdata" then return false end
        if type(handle.disconnect) ~= "function" then return false end
        return true
    )"));
}
