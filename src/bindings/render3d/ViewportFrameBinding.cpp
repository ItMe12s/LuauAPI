#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "framework/usertype/Usertype.hpp"
#include "render3d/CCViewportFrameNode.hpp"
#include "render3d/Transform3D.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <lualib.h>
#include <new>

namespace {
    using namespace luax;
    using namespace luax::render3d;

    constexpr char const* kTransformMeta = "luax.gd3d.Transform";
    constexpr char const* kMeshMeta = "luax.gd3d.Mesh";

    struct MeshHandle {
        std::uint64_t id = 0;
    };

    Transform* checkTransform(lua_State* L, int idx, [[maybe_unused]] char const* method) {
        return static_cast<Transform*>(luaL_checkudata(L, idx, kTransformMeta));
    }

    void pushTransform(lua_State* L, Transform const& transform) {
        auto* storage = static_cast<Transform*>(
            lua_newuserdatataggedwithmetatable(L, sizeof(Transform), luax::detail::transformTag())
        );
        new (storage) Transform(transform);
    }

    MeshHandle* checkMeshHandle(lua_State* L, int idx, [[maybe_unused]] char const* method) {
        return static_cast<MeshHandle*>(luaL_checkudata(L, idx, kMeshMeta));
    }

    std::uint64_t requireMeshId(lua_State* L, MeshHandle* handle, char const* method) {
        if (handle == nullptr || handle->id == 0) {
            luaL_error(L, "%s: mesh handle is invalid", method);
        }
        return handle->id;
    }

    int viewportNew(lua_State* L) {
        float const width = check<float>(L, 1, "gd3d.ViewportFrame.new");
        float const height = check<float>(L, 2, "gd3d.ViewportFrame.new");

        auto* node = CCViewportFrameNode::create(width, height);
        if (node == nullptr) {
            return pushNilErr(L, "failed to create ViewportFrame");
        }

        Usertype<CCViewportFrameNode>::pushOwned(L, node);
        return 1;
    }

    int viewportSetCamera(lua_State* L) {
        auto* self = Usertype<CCViewportFrameNode>::check(L, 1, "ViewportFrame:setCamera");
        auto const* transform = checkTransform(L, 2, "ViewportFrame:setCamera");
        float const fovY = check<float>(L, 3, "ViewportFrame:setCamera");
        float const zNear = check<float>(L, 4, "ViewportFrame:setCamera");
        float const zFar = check<float>(L, 5, "ViewportFrame:setCamera");

        self->setCamera3D(Camera3D{*transform, fovY, zNear, zFar});
        return 0;
    }

    int viewportGetCamera(lua_State* L) {
        auto const* self = Usertype<CCViewportFrameNode>::check(L, 1, "ViewportFrame:getCamera");
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
        auto* self = Usertype<CCViewportFrameNode>::check(L, 1, "ViewportFrame:addMesh");
        auto const meshId =
            requireMeshId(L, checkMeshHandle(L, 2, "ViewportFrame:addMesh"), "ViewportFrame:addMesh");
        auto const* transform = checkTransform(L, 3, "ViewportFrame:addMesh");

        push(L, self->addInstance(meshId, *transform));
        return 1;
    }

    int viewportSetInstanceTransform(lua_State* L) {
        auto* self = Usertype<CCViewportFrameNode>::check(L, 1, "ViewportFrame:setInstanceTransform");
        int const instanceId = check<int>(L, 2, "ViewportFrame:setInstanceTransform");
        auto const* transform = checkTransform(L, 3, "ViewportFrame:setInstanceTransform");

        push(L, self->setInstanceTransform(instanceId, *transform));
        return 1;
    }

    int viewportRemoveInstance(lua_State* L) {
        auto* self = Usertype<CCViewportFrameNode>::check(L, 1, "ViewportFrame:removeInstance");
        int const instanceId = check<int>(L, 2, "ViewportFrame:removeInstance");

        push(L, self->removeInstance(instanceId));
        return 1;
    }

    int viewportClearInstances(lua_State* L) {
        auto* self = Usertype<CCViewportFrameNode>::check(L, 1, "ViewportFrame:clearInstances");
        self->clearInstances();
        return 0;
    }
} // namespace

namespace luax {
    geode::Result<void> registerViewportFrame(lua_State* L) {
        auto const ccNodeTag = Usertype<cocos2d::CCNode>::tag();
        auto registerResult =
            Usertype<CCViewportFrameNode>::registerType(L, "ViewportFrame", {ccNodeTag});
        if (registerResult.isErr()) {
            return registerResult;
        }

        Usertype<CCViewportFrameNode>::method(L, "setCamera", &viewportSetCamera);
        Usertype<CCViewportFrameNode>::method(L, "getCamera", &viewportGetCamera);
        Usertype<CCViewportFrameNode>::method(L, "addMesh", &viewportAddMesh);
        Usertype<CCViewportFrameNode>::method(L, "setInstanceTransform", &viewportSetInstanceTransform);
        Usertype<CCViewportFrameNode>::method(L, "removeInstance", &viewportRemoveInstance);
        Usertype<CCViewportFrameNode>::method(L, "clearInstances", &viewportClearInstances);

        getOrCreateTable(L, "gd3d.ViewportFrame");
        setTableCFunction(L, -1, "new", &viewportNew);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(gd3d_viewport_frame_lib, registerViewportFrame)
#endif
