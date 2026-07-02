#include "core/Runtime.hpp"
#include "framework/usertype/Fields.hpp"
#include "framework/usertype/Usertype.hpp"
#include "host/lua_test_helpers.hpp"

#include <Geode/utils/cocos.hpp>
#include <RuntimeTypes.hpp>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <typeindex>

namespace {
    struct TestNode : cocos2d::CCNode {};

    struct AliasNode : TestNode {};

    struct OverflowNode : cocos2d::CCNode {};

    struct PlainObject : cocos2d::CCObject {};

    using RuntimeGuard = luauapi_test::FieldsRuntimeGuard;

    int getTestField(lua_State* L) {
        lua_pushliteral(L, "value");
        return 1;
    }

    int setTestField(lua_State* L) {
        (void)L;
        return 0;
    }

    int erroringGetField(lua_State* L) {
        luaL_error(L, "field getter boom");
        return 0;
    }

    int erroringSetField(lua_State* L) {
        luaL_error(L, "field setter boom");
        return 0;
    }

    int testMethod(lua_State* L) {
        lua_pushliteral(L, "called");
        return 1;
    }

    bool g_fieldAccessorRan = false;

    int trackingGetField(lua_State* L) {
        g_fieldAccessorRan = true;
        lua_pushliteral(L, "value");
        return 1;
    }

    int trackingSetField(lua_State* L) {
        g_fieldAccessorRan = true;
        return 0;
    }
} // namespace

TEST_CASE("UsertypeRegistry tagFor returns zero when tag limit is exceeded") {
    RuntimeGuard guard;
    auto& reg = luax::detail::UsertypeRegistry::get();
    reg.setNextTagForTests(LUA_UTAG_LIMIT);

    REQUIRE(reg.tagFor(std::type_index(typeid(OverflowNode))) == 0);
}

TEST_CASE("UsertypeRegistry returns error when tag limit is exceeded") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto& reg = luax::detail::UsertypeRegistry::get();
    reg.setNextTagForTests(LUA_UTAG_LIMIT);

    auto result = luax::Usertype<OverflowNode>::registerType(L, "OverflowNode");
    REQUIRE(result.isErr());
}

TEST_CASE("Usertype cannot push userdata when tag registration fails") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto& reg = luax::detail::UsertypeRegistry::get();
    reg.setNextTagForTests(LUA_UTAG_LIMIT);

    REQUIRE(luax::Usertype<OverflowNode>::registerType(L, "OverflowNode").isErr());
    REQUIRE(luax::Usertype<OverflowNode>::tag() == 0);

    auto* node = new OverflowNode();
    luax::Usertype<OverflowNode>::pushBorrowed(L, node);
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
    node->release();
}

TEST_CASE("UsertypeRegistry assigns unique tags") {
    RuntimeGuard guard;
    auto& reg = luax::detail::UsertypeRegistry::get();

    auto firstResult = reg.ensureInfo(std::type_index(typeid(TestNode)));
    auto secondResult = reg.ensureInfo(std::type_index(typeid(cocos2d::CCNode)));
    REQUIRE(firstResult.isOk());
    REQUIRE(secondResult.isOk());
    auto first = firstResult.unwrap()->tag;
    auto second = secondResult.unwrap()->tag;
    REQUIRE(first != 0);
    REQUIRE(second != 0);
    REQUIRE(first != second);
    REQUIRE(reg.tagFor(std::type_index(typeid(TestNode))) == first);
}

TEST_CASE("Usertype tag returns zero before registration") {
    RuntimeGuard guard;
    REQUIRE(luax::Usertype<TestNode>::tag() == 0);
}

TEST_CASE("Usertype registerType rejects invalid base tag") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto result = luax::Usertype<TestNode>::registerType(L, "TestNode", {0});
    REQUIRE(result.isErr());
}

TEST_CASE("Usertype registerType rejects multiple direct base tags") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<cocos2d::CCObject>::registerType(L, "CCObject").isOk());
    auto ccNodeTag = luax::Usertype<cocos2d::CCNode>::registerType(
        L, "CCNode", {luax::Usertype<cocos2d::CCObject>::tag()}
    );
    REQUIRE(ccNodeTag.isOk());

    auto result = luax::Usertype<TestNode>::registerType(
        L,
        "TestNode",
        {luax::Usertype<cocos2d::CCObject>::tag(), luax::Usertype<cocos2d::CCNode>::tag()}
    );
    REQUIRE(result.isErr());
}

TEST_CASE("Usertype pushBorrowed returns nil when metatable was never created") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto& reg = luax::detail::UsertypeRegistry::get();
    auto infoResult = reg.ensureInfo(std::type_index(typeid(TestNode)));
    REQUIRE(infoResult.isOk());
    auto& info = *infoResult.unwrap();
    info.name = "TestNodeUnregisteredMt";
    info.mtName = "luax:TestNodeUnregisteredMt";

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
    node->release();
}

TEST_CASE("Usertype pushBorrowed returns nil when type is not registered") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    REQUIRE(lua_isnil(L, -1));
    lua_pop(L, 1);
    node->release();
}

TEST_CASE("Usertype metatable dispatches methods and fields") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    auto result = luax::Usertype<TestNode>::registerType(L, "TestNode");
    REQUIRE(result.isOk());

    luax::Usertype<TestNode>::method(L, "ping", &testMethod);
    luax::Usertype<TestNode>::field(L, "answer", &getTestField, &setTestField);

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    REQUIRE(lua_isuserdata(L, -1));

    lua_getfield(L, -1, "ping");
    REQUIRE(lua_isfunction(L, -1));
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
    REQUIRE(std::string_view(lua_tostring(L, -1)) == "called");
    lua_pop(L, 1);

    lua_getfield(L, -1, "answer");
    REQUIRE(lua_isstring(L, -1));
    REQUIRE(std::string_view(lua_tostring(L, -1)) == "value");
    lua_pop(L, 2);

    node->release();
}

TEST_CASE(
    "Usertype __index returns nil for unknown field without "
    "materializing m_fields"
) {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);

    lua_getfield(L, -1, "missingField");
    REQUIRE(lua_isnil(L, -1));
    REQUIRE_FALSE(luax::Fields::tryPush(L, node));

    lua_pop(L, 2);
    node->release();
}

TEST_CASE("Usertype rejects writes to read-only m_fields") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());

    auto* node = new TestNode();
    lua_pushcfunction(
        L,
        [](lua_State* S) -> int {
            lua_setfield(S, 1, "m_fields");
            return 0;
        },
        "assignMFields"
    );
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    lua_newtable(L);
    REQUIRE(lua_pcall(L, 2, 0, 0) != 0);
    lua_pop(L, 1);

    node->release();
}

TEST_CASE("Usertype field getter uses traceback when runtime is not ready") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());
    luax::Usertype<TestNode>::field(L, "boomField", &erroringGetField, &setTestField);

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);

    runtime->clearLastError();
    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::NotReady);

    lua_pushcfunction(
        L,
        [](lua_State* S) -> int {
            lua_getfield(S, 1, "boomField");
            lua_pop(S, 1);
            return 0;
        },
        "getBoomField"
    );
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    lua_pop(L, 1);

    auto const& err = runtime->lastError();
    REQUIRE(err.find("field getter boom") != std::string::npos);
    REQUIRE(err.find("stack:") != std::string::npos);

    lua_pop(L, 1);
    node->release();
}

TEST_CASE("Usertype field setter uses traceback when runtime is not ready") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());
    luax::Usertype<TestNode>::field(L, "boomField", &getTestField, &erroringSetField);

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);

    runtime->clearLastError();
    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::NotReady);

    lua_pushcfunction(
        L,
        [](lua_State* S) -> int {
            lua_pushliteral(S, "value");
            lua_setfield(S, 1, "boomField");
            return 0;
        },
        "assignBoomField"
    );
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    lua_pop(L, 1);

    auto const& err = runtime->lastError();
    REQUIRE(err.find("field setter boom") != std::string::npos);
    REQUIRE(err.find("stack:") != std::string::npos);

    lua_pop(L, 1);
    node->release();
}

TEST_CASE("Usertype field getter skips accessor when runtime panicked") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());
    luax::Usertype<TestNode>::field(L, "trackedField", &trackingGetField, &setTestField);

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);

    g_fieldAccessorRan = false;
    runtime->clearLastError();
    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::Panicked);

    lua_pushcfunction(
        L,
        [](lua_State* S) -> int {
            lua_getfield(S, 1, "trackedField");
            lua_pop(S, 1);
            return 0;
        },
        "getTrackedField"
    );
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    lua_pop(L, 1);
    REQUIRE_FALSE(g_fieldAccessorRan);

    auto const& err = runtime->lastError();
    REQUIRE(err.find("luau runtime panicked") != std::string::npos);
    REQUIRE(err.find("stack:") == std::string::npos);

    lua_pop(L, 1);
    node->release();
}

TEST_CASE("Usertype field setter skips accessor when runtime panicked") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<TestNode>::registerType(L, "TestNode").isOk());
    luax::Usertype<TestNode>::field(L, "trackedField", &getTestField, &trackingSetField);

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);

    g_fieldAccessorRan = false;
    runtime->clearLastError();
    runtime->setStatusForTests(imes::luauapi::RuntimeStatus::Panicked);

    lua_pushcfunction(
        L,
        [](lua_State* S) -> int {
            lua_pushliteral(S, "value");
            lua_setfield(S, 1, "trackedField");
            return 0;
        },
        "assignTrackedField"
    );
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    lua_pop(L, 1);
    REQUIRE_FALSE(g_fieldAccessorRan);

    auto const& err = runtime->lastError();
    REQUIRE(err.find("luau runtime panicked") != std::string::npos);
    REQUIRE(err.find("stack:") == std::string::npos);

    lua_pop(L, 1);
    node->release();
}

TEST_CASE("Usertype pushBorrowedDynamic preserves runtime subclass type") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(luax::Usertype<cocos2d::CCObject>::registerType(L, "CCObject").isOk());
    REQUIRE(
        luax::Usertype<TestNode>::registerType(L, "TestNode", {luax::Usertype<cocos2d::CCObject>::tag()})
            .isOk()
    );

    luax::Usertype<TestNode>::method(L, "ping", &testMethod);

    auto* node = new TestNode();
    luax::Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, node);
    REQUIRE(lua_isuserdata(L, -1));

    lua_getfield(L, -1, "ping");
    REQUIRE(lua_isfunction(L, -1));
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
    REQUIRE(std::string_view(lua_tostring(L, -1)) == "called");
    lua_pop(L, 2);

    node->release();
}

TEST_CASE("tryNodeCandidate accepts CCNode-derived userdata from registered metadata") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<cocos2d::CCObject>::registerType(L, "CCObject").isOk());
    REQUIRE(
        luax::Usertype<TestNode>::registerType(L, "TestNode", {luax::Usertype<cocos2d::CCObject>::tag()})
            .isOk()
    );

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    REQUIRE(luax::detail::tryNodeCandidate(L, -1) == node);

    luax::Fields::push(L, luax::detail::tryNodeCandidate(L, -1));
    REQUIRE(lua_istable(L, -1));
    lua_pushliteral(L, "marker");
    lua_setfield(L, -2, "token");
    lua_pop(L, 1);

    lua_getfield(L, -1, "m_fields");
    REQUIRE(lua_istable(L, -1));
    lua_getfield(L, -1, "token");
    REQUIRE(std::string_view(lua_tostring(L, -1)) == "marker");
    lua_pop(L, 3);

    node->release();
}

TEST_CASE("tryNodeCandidate rejects non-CCNode userdata") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<PlainObject>::registerType(L, "PlainObject").isOk());

    auto* obj = new PlainObject();
    luax::Usertype<PlainObject>::pushBorrowed(L, obj);
    REQUIRE(luax::detail::tryNodeCandidate(L, -1) == nullptr);
    lua_pop(L, 1);
    obj->release();
}

TEST_CASE("findPushTypeInfo falls back to CCNode for unknown node subclasses") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(luax::Usertype<cocos2d::CCObject>::registerType(L, "CCObject").isOk());
    REQUIRE(
        luax::Usertype<cocos2d::CCNode>::registerType(
            L, "CCNode", {luax::Usertype<cocos2d::CCObject>::tag()}
        )
            .isOk()
    );
    luax::Usertype<cocos2d::CCNode>::method(L, "nodePing", &testMethod);

    auto* node = new AliasNode();
    auto const* info = luax::detail::findPushTypeInfo(node, std::type_index(typeid(cocos2d::CCNode)));
    REQUIRE(info != nullptr);
    REQUIRE(info->name == "CCNode");

    luax::Usertype<cocos2d::CCNode>::pushBorrowedDynamic(L, node);
    REQUIRE(lua_isuserdata(L, -1));

    lua_getfield(L, -1, "nodePing");
    REQUIRE(lua_isfunction(L, -1));
    lua_pop(L, 2);

    node->release();
}

TEST_CASE("findPushTypeInfo resolves registered runtime type name") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(luax::Usertype<cocos2d::CCObject>::registerType(L, "CCObject").isOk());
    REQUIRE(
        luax::Usertype<TestNode>::registerType(L, "TestNode", {luax::Usertype<cocos2d::CCObject>::tag()})
            .isOk()
    );
    luax::Usertype<TestNode>::method(L, "ping", &testMethod);

    auto* node = new AliasNode();
    geode::cocos::setObjectNameForTests(node, "cocos2d::TestNode");
    auto const* info =
        luax::detail::findPushTypeInfo(node, std::type_index(typeid(cocos2d::CCObject)));
    REQUIRE(info != nullptr);
    REQUIRE(info->name == "TestNode");

    luax::Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, node);
    REQUIRE(lua_isuserdata(L, -1));

    lua_getfield(L, -1, "ping");
    REQUIRE(lua_isfunction(L, -1));
    lua_pop(L, 2);

    node->release();
}

TEST_CASE("dropBorrowedTargetIfFinalRelease is no-op without a borrowed entry") {
    RuntimeGuard guard;
    auto* node = new TestNode();

    REQUIRE_NOTHROW(luax::dropBorrowedTargetIfFinalRelease(node));

    node->release();
}

TEST_CASE("dropBorrowedTargetIfFinalRelease is no-op for non-node CCObject") {
    RuntimeGuard guard;
    auto* object = new PlainObject();

    REQUIRE_NOTHROW(luax::dropBorrowedTargetIfFinalRelease(object));

    object->release();
}

TEST_CASE("dropBorrowedTargetIfFinalRelease defers drop while retainCount is above one") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<cocos2d::CCObject>::registerType(L, "CCObject").isOk());
    REQUIRE(
        luax::Usertype<TestNode>::registerType(L, "TestNode", {luax::Usertype<cocos2d::CCObject>::tag()})
            .isOk()
    );

    auto* node = new TestNode();
    node->retain();

    luax::Usertype<TestNode>::pushBorrowed(L, node);
    REQUIRE(luax::detail::tryCandidate(L, -1).obj == node);

    luax::dropBorrowedTargetIfFinalRelease(node);
    REQUIRE(luax::detail::tryCandidate(L, -1).obj == node);

    node->release();
    luax::dropBorrowedTargetIfFinalRelease(node);
    REQUIRE(luax::detail::tryCandidate(L, -1).obj == nullptr);

    lua_pop(L, 1);
    node->release();
}

TEST_CASE("borrowed userdata rejects access after borrowed target is dropped") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<cocos2d::CCObject>::registerType(L, "CCObject").isOk());
    REQUIRE(
        luax::Usertype<TestNode>::registerType(L, "TestNode", {luax::Usertype<cocos2d::CCObject>::tag()})
            .isOk()
    );

    auto* node = new TestNode();
    luax::Usertype<TestNode>::pushBorrowed(L, node);
    REQUIRE(lua_isuserdata(L, -1));
    REQUIRE(luax::detail::tryCandidate(L, -1).obj == node);

    luax::dropBorrowedTargetIfFinalRelease(node);
    REQUIRE(luax::detail::tryCandidate(L, -1).obj == nullptr);

    lua_pushcfunction(
        L,
        [](lua_State* S) -> int {
            luax::Usertype<TestNode>::check(S, 1, "test");
            return 0;
        },
        "checkBorrowed"
    );
    lua_pushvalue(L, -2);
    REQUIRE(lua_pcall(L, 1, 0, 0) != 0);
    REQUIRE(std::string_view(lua_tostring(L, -1)).find("expected live") != std::string_view::npos);
    lua_pop(L, 2);

    node->release();
}

TEST_CASE("tryNodeCandidate accepts CCNode lower-bound dynamic userdata") {
    RuntimeGuard guard;
    auto* runtime = luax::Runtime::getOrCreate();
    auto* L = runtime->state();

    REQUIRE(luax::Usertype<cocos2d::CCObject>::registerType(L, "CCObject").isOk());
    REQUIRE(
        luax::Usertype<cocos2d::CCNode>::registerType(
            L, "CCNode", {luax::Usertype<cocos2d::CCObject>::tag()}
        )
            .isOk()
    );

    auto* node = new AliasNode();
    luax::Usertype<cocos2d::CCNode>::pushBorrowedDynamic(L, node);
    REQUIRE(luax::detail::tryNodeCandidate(L, -1) == node);

    luax::Fields::push(L, luax::detail::tryNodeCandidate(L, -1));
    REQUIRE(lua_istable(L, -1));
    lua_pushliteral(L, "marker");
    lua_setfield(L, -2, "token");
    lua_pop(L, 1);

    lua_getfield(L, -1, "m_fields");
    REQUIRE(lua_istable(L, -1));
    lua_getfield(L, -1, "token");
    REQUIRE(std::string_view(lua_tostring(L, -1)) == "marker");
    lua_pop(L, 3);

    node->release();
}
