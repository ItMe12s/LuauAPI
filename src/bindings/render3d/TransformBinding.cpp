#include "bindings/render3d/internal/Marshaling.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "render3d/types/Transform3D.hpp"

#include <Geode/Geode.hpp>
#include <glm/vec3.hpp>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;
    using namespace luax::gd3d;
    using Transform = render3d::Transform;

    int transformNew(lua_State* L) {
        int const argc = lua_gettop(L);
        if (argc == 0) {
            pushTransform(L, Transform::identity());
            return 1;
        }

        auto const pos = checkVec3(L, 1, "gd3d.Transform.new");
        if (argc >= 2) {
            auto const lookAt = checkVec3(L, 2, "gd3d.Transform.new");
            pushTransform(L, Transform::fromLookAt(pos, lookAt));
            return 1;
        }

        pushTransform(L, Transform{pos, glm::quat(1.0f, 0.0f, 0.0f, 0.0f)});
        return 1;
    }

    int transformFromEuler(lua_State* L) {
        float const pitch = check<float>(L, 1, "gd3d.Transform.fromEuler");
        float const yaw = check<float>(L, 2, "gd3d.Transform.fromEuler");
        float const roll = check<float>(L, 3, "gd3d.Transform.fromEuler");
        pushTransform(L, Transform::fromEuler(pitch, yaw, roll));
        return 1;
    }

    int transformFromAxisAngle(lua_State* L) {
        auto const axis = checkVec3(L, 1, "gd3d.Transform.fromAxisAngle");
        float const angle = check<float>(L, 2, "gd3d.Transform.fromAxisAngle");
        pushTransform(L, Transform::fromAxisAngle(axis, angle));
        return 1;
    }

    int transformMul(lua_State* L) {
        auto const* lhs = checkTransform(L, 1, "Transform.__mul");
        auto const* rhs = checkTransform(L, 2, "Transform.__mul");
        pushTransform(L, (*lhs) * (*rhs));
        return 1;
    }

    int transformInverse(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:inverse");
        pushTransform(L, self->inverse());
        return 1;
    }

    int transformLerp(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:lerp");
        auto const* goal = checkTransform(L, 2, "Transform:lerp");
        float const alpha = check<float>(L, 3, "Transform:lerp");
        pushTransform(L, self->lerp(*goal, alpha));
        return 1;
    }

    int transformPosition(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:position");
        pushVec3(L, self->position);
        return 1;
    }

    int transformLookVector(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:lookVector");
        pushVec3(L, self->lookVector());
        return 1;
    }

    int transformRightVector(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:rightVector");
        pushVec3(L, self->rightVector());
        return 1;
    }

    int transformUpVector(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:upVector");
        pushVec3(L, self->upVector());
        return 1;
    }

    int transformWithPosition(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:withPosition");
        auto const pos = checkVec3(L, 2, "Transform:withPosition");
        pushTransform(L, self->withPosition(pos));
        return 1;
    }

    int transformWithRotation(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:withRotation");
        auto const* rot = checkTransform(L, 2, "Transform:withRotation");
        pushTransform(L, self->withRotationOf(*rot));
        return 1;
    }

    int transformEulerAngles(lua_State* L) {
        auto const* self = checkTransform(L, 1, "Transform:eulerAngles");
        pushVec3(L, self->eulerAngles());
        return 1;
    }

    void registerTransformMetatable(lua_State* L) {
        luaL_Reg const methods[] = {
            {"inverse", transformInverse},
            {"lerp", transformLerp},
            {"position", transformPosition},
            {"lookVector", transformLookVector},
            {"rightVector", transformRightVector},
            {"upVector", transformUpVector},
            {"withPosition", transformWithPosition},
            {"withRotation", transformWithRotation},
            {"eulerAngles", transformEulerAngles},
            {"__mul", transformMul},
            {nullptr, nullptr},
        };

        registerTaggedMetatable(
            L, kTransformMeta, detail::transformTag(), methods, std::nullopt, std::nullopt, kTransformTypeName
        );
    }
} // namespace

namespace luax {
    geode::Result<void> registerTransform(lua_State* L) {
        registerTransformMetatable(L);

        getOrCreateTable(L, "gd3d.Transform");
        setTableCFunction(L, -1, "new", &transformNew);
        setTableCFunction(L, -1, "fromEuler", &transformFromEuler);
        setTableCFunction(L, -1, "fromAxisAngle", &transformFromAxisAngle);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax
