#include "framework/Binding.hpp"
#include "lua_test_helpers.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>

namespace luax {
    geode::Result<void> registerGd3d(lua_State* L);
} // namespace luax

namespace {
    using Catch::Approx;
    using luauapi_test::BindingGuard;
    using luauapi_test::makeLuaState;
    using luauapi_test::runScriptReturnsBool;
    using luauapi_test::runScriptReturnsNumber;

    void registerGd3dBindings(lua_State* L) {
        luax::registerBinding({"gd3d", &luax::registerGd3d, 0});
        REQUIRE(luax::applyAllBindings(L) == std::nullopt);
    }

    bool gd3dFieldIsNil(lua_State* L, char const* field) {
        lua_getglobal(L, "gd3d");
        if (!lua_istable(L, -1)) {
            lua_settop(L, 0);
            return true;
        }
        lua_getfield(L, -1, field);
        bool const isNil = lua_isnil(L, -1);
        lua_settop(L, 0);
        return isNil;
    }

    bool gd3dTransformFieldIsFunction(lua_State* L, char const* field) {
        lua_getglobal(L, "gd3d");
        REQUIRE(lua_istable(L, -1));
        lua_getfield(L, -1, "Transform");
        REQUIRE(lua_istable(L, -1));
        lua_getfield(L, -1, field);
        bool const isFn = lua_isfunction(L, -1);
        lua_settop(L, 0);
        return isFn;
    }

    char const* kTransformNearHelper = R"(
local function vec3Near(a, b, eps)
    eps = eps or 1e-4
    local dx = a.x - b.x
    local dy = a.y - b.y
    local dz = a.z - b.z
    return (dx * dx + dy * dy + dz * dz) <= eps * eps
end

local function transformNear(a, b, eps)
    return vec3Near(a:position(), b:position(), eps)
end

local function abs(v)
    if v < 0 then return -v end
    return v
end

local function dot(a, b)
    return a.x * b.x + a.y * b.y + a.z * b.z
end

local function basisLooksValid(t)
    local right = t:rightVector()
    local up = t:upVector()
    local look = t:lookVector()
    return abs(dot(right, right) - 1) <= 1e-3
        and abs(dot(up, up) - 1) <= 1e-3
        and abs(dot(look, look) - 1) <= 1e-3
        and abs(dot(right, up)) <= 1e-3
        and abs(dot(right, look)) <= 1e-3
        and abs(dot(up, look)) <= 1e-3
end
)";
} // namespace

TEST_CASE("registerGd3d under host exposes gd3d.Transform") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    REQUIRE(gd3dTransformFieldIsFunction(L.get(), "new"));
    REQUIRE(gd3dTransformFieldIsFunction(L.get(), "fromEuler"));
    REQUIRE(gd3dTransformFieldIsFunction(L.get(), "fromAxisAngle"));
}

TEST_CASE("registerGd3d under host omits non-host gd3d APIs") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    REQUIRE(gd3dFieldIsNil(L.get(), "texture"));
    REQUIRE(gd3dFieldIsNil(L.get(), "Material"));
    REQUIRE(gd3dFieldIsNil(L.get(), "ViewportFrame"));
}

TEST_CASE("gd3d.Transform.new defaults to identity") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    auto x = runScriptReturnsNumber(L.get(), R"(
        local t = gd3d.Transform.new()
        return t:position().x
    )");
    auto y = runScriptReturnsNumber(L.get(), R"(
        local t = gd3d.Transform.new()
        return t:position().y
    )");
    auto z = runScriptReturnsNumber(L.get(), R"(
        local t = gd3d.Transform.new()
        return t:position().z
    )");

    REQUIRE(x.has_value());
    REQUIRE(y.has_value());
    REQUIRE(z.has_value());
    REQUIRE(*x == Approx(0.0).margin(1e-4));
    REQUIRE(*y == Approx(0.0).margin(1e-4));
    REQUIRE(*z == Approx(0.0).margin(1e-4));

    REQUIRE(runScriptReturnsBool(L.get(), R"(
        local t = gd3d.Transform.new()
        local id = gd3d.Transform.new()
        return (t * id:inverse()):position().x == 0
            and (t * id:inverse()):position().y == 0
            and (t * id:inverse()):position().z == 0
    )"));
}

TEST_CASE("gd3d.Transform.fromEuler produces usable transform") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    REQUIRE(runScriptReturnsBool(L.get(), std::string(kTransformNearHelper) + R"(
        local t = gd3d.Transform.fromEuler(0.3, -0.8, 0.1)
        return basisLooksValid(t)
            and transformNear(t * t:inverse(), gd3d.Transform.new())
    )"));
}

TEST_CASE("gd3d.Transform.fromAxisAngle produces usable transform") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    REQUIRE(runScriptReturnsBool(L.get(), std::string(kTransformNearHelper) + R"(
        local t = gd3d.Transform.fromAxisAngle({ x = 0, y = 1, z = 0 }, 1.1)
        local look = t:lookVector()
        return basisLooksValid(t)
            and abs((look.x * look.x + look.y * look.y + look.z * look.z) - 1.0) <= 1e-3
            and abs(look.x) > 0.5
    )"));
}

TEST_CASE("gd3d.Transform __mul composes transforms") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    REQUIRE(runScriptReturnsBool(L.get(), std::string(kTransformNearHelper) + R"(
        local a = gd3d.Transform.fromEuler(0.3, -0.8, 0.1):withPosition({ x = 1, y = 2, z = 3 })
        local b = gd3d.Transform.fromAxisAngle({ x = 0, y = 1, z = 0 }, 1.1):withPosition({ x = -1, y = 0.5, z = 2 })
        local c = gd3d.Transform.fromEuler(0, 0, 0):withPosition({ x = 0, y = 0, z = 5 })
        return transformNear((a * b) * c, a * (b * c))
    )"));
}

TEST_CASE("gd3d.Transform inverse undoes composition") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    REQUIRE(runScriptReturnsBool(L.get(), std::string(kTransformNearHelper) + R"(
        local a = gd3d.Transform.fromEuler(0.3, -0.8, 0.1):withPosition({ x = 1, y = 0, z = -3 })
        local b = gd3d.Transform.fromAxisAngle({ x = 0, y = 1, z = 0 }, 0.7):withPosition({ x = -2, y = 4, z = 1 })
        local composed = a * b
        local identity = gd3d.Transform.new()
        return transformNear(composed * composed:inverse(), identity)
            and transformNear(composed:inverse() * composed, identity)
    )"));
}

TEST_CASE("gd3d.Transform lerp hits endpoints") {
    BindingGuard guard;
    auto L = makeLuaState();
    registerGd3dBindings(L.get());

    REQUIRE(runScriptReturnsBool(L.get(), std::string(kTransformNearHelper) + R"(
        local start = gd3d.Transform.new()
        local goal = gd3d.Transform.fromEuler(0, 1.5707963, 0):withPosition({ x = 4, y = -2, z = 1 })
        return transformNear(start:lerp(goal, 0), start)
            and transformNear(start:lerp(goal, 1), goal)
    )"));
}
