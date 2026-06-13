#include "bindings/geode/web/WebInternal.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <thread>

namespace {
    using namespace luax;

    struct WebGuard {
        WebGuard() {
            Runtime::setMainThreadId(std::this_thread::get_id());
            resetBindingsForTests();
        }

        ~WebGuard() {
            clearWebState();
            Runtime::resetForTests();
            resetBindingsForTests();
        }
    };

    std::size_t countLiveTasks() {
        return activeTasks().compactAndCountLive();
    }

    std::size_t countLiveListeners() {
        return activeListeners().compactAndCountLive();
    }
} // namespace

TEST_CASE("runtime reset clears global web state") {
    WebGuard guard;
    Runtime::getOrCreate();

    activeTasks().track(std::make_shared<WebTask>(1));
    activeListeners().track(std::make_shared<WebListenerState>());
    ensureShutdownHook();

    REQUIRE(countLiveTasks() >= 1);
    REQUIRE(countLiveListeners() >= 1);
    REQUIRE(webShutdownHookRegistered());

    Runtime::resetForTests();
    Runtime::setMainThreadId(std::this_thread::get_id());

    REQUIRE(activeTasks().empty());
    REQUIRE(activeListeners().empty());
    REQUIRE_FALSE(webShutdownHookRegistered());
}

TEST_CASE("web shutdown hook cancels live tasks and disconnects listeners") {
    WebGuard guard;
    Runtime::getOrCreate();

    auto task = std::make_shared<WebTask>(42);
    auto listener = std::make_shared<WebListenerState>();
    listener->connected = true;

    activeTasks().track(task);
    activeListeners().track(listener);
    ensureShutdownHook();

    clearWebState();

    REQUIRE(task->done);
    REQUIRE_FALSE(listener->connected);
    REQUIRE(activeTasks().empty());
    REQUIRE(activeListeners().empty());
    REQUIRE_FALSE(webShutdownHookRegistered());
}
