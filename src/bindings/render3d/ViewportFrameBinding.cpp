#include "bindings/render3d/Gd3dShared.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "framework/usertype/Usertype.hpp"
#include "render3d/CCViewportFrame.hpp"
#include "render3d/MeshAsset.hpp"
#include "render3d/Transform3D.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;
    using namespace luax::gd3d;
    using namespace luax::render3d;

    int viewportNew(lua_State* L) {
        float const width = check<float>(L, 1, "gd3d.ViewportFrame.new");
        float const height = check<float>(L, 2, "gd3d.ViewportFrame.new");

        auto* node = CCViewportFrame::create(width, height);
        if (node == nullptr) {
            return pushNilErr(L, "failed to create ViewportFrame");
        }

        Usertype<CCViewportFrame>::pushOwned(L, node);
        return 1;
    }

    int viewportSetCamera(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setCamera");
        auto const* transform = checkTransform(L, 2, "ViewportFrame:setCamera");
        float const fovY = check<float>(L, 3, "ViewportFrame:setCamera");
        float const zNear = check<float>(L, 4, "ViewportFrame:setCamera");
        float const zFar = check<float>(L, 5, "ViewportFrame:setCamera");

        self->setCamera3D(Camera3D{*transform, fovY, zNear, zFar});
        return 0;
    }

    int viewportGetCamera(lua_State* L) {
        auto const* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:getCamera");
        Camera3D const& camera = self->getCamera3D();

        lua_createtable(L, 0, 4);
        pushTransform(L, camera.transform);
        lua_setfield(L, -2, "transform");
        push(L, camera.fovYDegrees);
        lua_setfield(L, -2, "fovY");
        push(L, camera.zNear);
        lua_setfield(L, -2, "near");
        push(L, camera.zFar);
        lua_setfield(L, -2, "far");
        return 1;
    }

    int viewportAddMesh(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:addMesh");
        auto const meshId =
            requireMeshId(L, checkMeshHandle(L, 2, "ViewportFrame:addMesh"), "ViewportFrame:addMesh");
        auto const* transform = checkTransform(L, 3, "ViewportFrame:addMesh");

        auto mesh = MeshRegistry::instance().get(meshId);
        if (mesh == nullptr) {
            luaL_error(L, "ViewportFrame:addMesh: mesh handle is invalid");
        }

        int const instanceId = self->addInstance(meshId, std::move(mesh), *transform);
        if (!lua_isnoneornil(L, 4)) {
            self->setInstanceMaterial(instanceId, requireMaterial(L, 4, "ViewportFrame:addMesh"));
        }
        push(L, instanceId);
        return 1;
    }

    int viewportSetInstanceMaterial(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setInstanceMaterial");
        int const instanceId = check<int>(L, 2, "ViewportFrame:setInstanceMaterial");

        std::shared_ptr<Material> material{};
        if (!lua_isnoneornil(L, 3)) {
            material = requireMaterial(L, 3, "ViewportFrame:setInstanceMaterial");
        }

        push(L, self->setInstanceMaterial(instanceId, std::move(material)));
        return 1;
    }

    int viewportSetInstanceColor(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setInstanceColor");
        int const instanceId = check<int>(L, 2, "ViewportFrame:setInstanceColor");
        auto const color = checkVec3(L, 3, "ViewportFrame:setInstanceColor");

        push(L, self->setInstanceColor(instanceId, color));
        return 1;
    }

    int viewportSetInstanceTransform(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setInstanceTransform");
        int const instanceId = check<int>(L, 2, "ViewportFrame:setInstanceTransform");
        auto const* transform = checkTransform(L, 3, "ViewportFrame:setInstanceTransform");

        push(L, self->setInstanceTransform(instanceId, *transform));
        return 1;
    }

    int viewportRemoveInstance(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:removeInstance");
        int const instanceId = check<int>(L, 2, "ViewportFrame:removeInstance");

        push(L, self->removeInstance(instanceId));
        return 1;
    }

    int viewportClearInstances(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:clearInstances");
        self->clearInstances();
        return 0;
    }
} // namespace

namespace luax {
    geode::Result<void> registerViewportFrame(lua_State* L) {
        auto const ccNodeTag = Usertype<cocos2d::CCNode>::tag();
        auto registerResult =
            Usertype<CCViewportFrame>::registerType(L, "ViewportFrame", {ccNodeTag});
        if (registerResult.isErr()) {
            return registerResult;
        }

        Usertype<CCViewportFrame>::method(L, "setCamera", &viewportSetCamera);
        Usertype<CCViewportFrame>::method(L, "getCamera", &viewportGetCamera);
        Usertype<CCViewportFrame>::method(L, "addMesh", &viewportAddMesh);
        Usertype<CCViewportFrame>::method(L, "setInstanceMaterial", &viewportSetInstanceMaterial);
        Usertype<CCViewportFrame>::method(L, "setInstanceColor", &viewportSetInstanceColor);
        Usertype<CCViewportFrame>::method(L, "setInstanceTransform", &viewportSetInstanceTransform);
        Usertype<CCViewportFrame>::method(L, "removeInstance", &viewportRemoveInstance);
        Usertype<CCViewportFrame>::method(L, "clearInstances", &viewportClearInstances);

        getOrCreateTable(L, "gd3d.ViewportFrame");
        setTableCFunction(L, -1, "new", &viewportNew);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(gd3d_viewport_frame_lib, registerViewportFrame)
#endif
