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

namespace {
    using imes::luauapi::LuaScriptEvent;
    using imes::luauapi::postScriptEvent;
    using luauapi_test::collectGarbage;
    using luauapi_test::runScriptPcall;
    using luauapi_test::runScriptReturnsBool;

    struct ScriptEventTestGuard {
        ScriptEventTestGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            geode::test::bindMainThreadToCurrent();
            luax::resetBindingsForTests();
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

    handle = {};
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

    handle = {};
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

    handle = {};
}

TEST_CASE("postScriptEvent delivers to Lua listeners on the main thread") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        geode.ScriptEvent.listen(function(topic, payload)
            seen_topic = topic
            seen_payload = payload
        end)
    )"));

    postScriptEvent("cxx.topic", "cxx.payload");

    REQUIRE(runScriptReturnsBool(L, R"(
        return seen_topic == "cxx.topic" and seen_payload == "cxx.payload"
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
        lua_seen = nil
        geode.ScriptEvent.listen(function()
            lua_seen = true
        end)
    )"));

    postScriptEvent("both.directions", "");

    REQUIRE(cppCalls == 1);
    REQUIRE(runScriptReturnsBool(L, "return lua_seen == true"));

    handle = {};
}

TEST_CASE("geode.ScriptEvent.listenFor filters by topic") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        alpha_seen = nil
        beta_seen = nil
        geode.ScriptEvent.listenFor("alpha", function(topic, payload)
            alpha_seen = payload
        end)
        geode.ScriptEvent.listenFor("beta", function(topic, payload)
            beta_seen = payload
        end)
    )"));

    postScriptEvent("beta", "b1");
    REQUIRE(runScriptReturnsBool(L, R"(
        return alpha_seen == nil and beta_seen == "b1"
    )"));

    postScriptEvent("alpha", "a1");
    REQUIRE(runScriptReturnsBool(L, R"(
        return alpha_seen == "a1" and beta_seen == "b1"
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
        only_this_calls = calls
    )"));

    postScriptEvent("other.topic", "");
    REQUIRE(runScriptReturnsBool(L, "return only_this_calls == 0"));
}

TEST_CASE("listener handle disconnect stops delivery") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        disconnect_seen = nil
        local handle = geode.ScriptEvent.listen(function()
            disconnect_seen = (disconnect_seen or 0) + 1
        end)
        handle:disconnect()
    )"));

    postScriptEvent("gone.listener", "");
    REQUIRE(runScriptReturnsBool(L, "return disconnect_seen == nil"));
}

TEST_CASE("listener handle garbage collection disconnects") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        gc_seen = nil
        local function makeListener()
            geode.ScriptEvent.listen(function()
                gc_seen = (gc_seen or 0) + 1
            end)
        end
        makeListener()
    )"));
    collectGarbage(L);

    postScriptEvent("gc.listener", "");
    REQUIRE(runScriptReturnsBool(L, "return gc_seen == nil"));
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
        seen_order = order
    )"));

    postScriptEvent("ordered", "");
    REQUIRE(runScriptReturnsBool(L, R"(
        return seen_order[1] == "early" and seen_order[2] == "late"
    )"));
}

TEST_CASE("listener returning true stops later listeners") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        local calls = 0
        geode.ScriptEvent.listen(function()
            calls = calls + 1
            return true
        end)
        geode.ScriptEvent.listen(function()
            calls = calls + 1
        end)
        stop_calls = calls
    )"));

    postScriptEvent("stopped", "");
    REQUIRE(runScriptReturnsBool(L, "return stop_calls == 1"));
}

TEST_CASE("off-main postScriptEvent hops to the main thread") {
    ScriptEventTestGuard guard;
    auto* L = guard.makeState();

    REQUIRE(runScriptPcall(L, R"(
        threaded_topic = nil
        threaded_payload = nil
        geode.ScriptEvent.listen(function(topic, payload)
            threaded_topic = topic
            threaded_payload = payload
        end)
    )"));

    std::thread worker([] {
        postScriptEvent("thread.topic", "thread.payload");
    });
    worker.join();

    REQUIRE(runScriptReturnsBool(L, "return threaded_topic == nil"));

    geode::test::drainMainThreadQueue();

    REQUIRE(runScriptReturnsBool(L, R"(
        return threaded_topic == "thread.topic" and threaded_payload == "thread.payload"
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