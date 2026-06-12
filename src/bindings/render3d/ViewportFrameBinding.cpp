#include "bindings/render3d/Gd3dShared.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"
#include "framework/usertype/Usertype.hpp"
#include "render3d/viewport/CCViewportFrame.hpp"
#include "render3d/assets/MeshAsset.hpp"
#include "render3d/types/Transform3D.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
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

    int viewportSetBackgroundColor(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setBackgroundColor");
        auto const color = parseColor(L, 2, "ViewportFrame:setBackgroundColor");

        auto settings = self->renderSettings();
        settings.clearColor = color;
        self->setRenderSettings(settings);
        return 0;
    }

    int viewportGetBackgroundColor(lua_State* L) {
        auto const* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:getBackgroundColor");
        auto const& color = self->renderSettings().clearColor;

        lua_createtable(L, 0, 4);
        push(L, color.r);
        lua_setfield(L, -2, "r");
        push(L, color.g);
        lua_setfield(L, -2, "g");
        push(L, color.b);
        lua_setfield(L, -2, "b");
        push(L, color.a);
        lua_setfield(L, -2, "a");
        return 1;
    }

    int viewportSetLight(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setLight");
        auto const direction = checkVec3(L, 2, "ViewportFrame:setLight");
        if (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f) {
            luaL_error(L, "ViewportFrame:setLight: direction must be non-zero");
        }

        glm::vec3 color{1.0f, 1.0f, 1.0f};
        if (!lua_isnoneornil(L, 3)) {
            color = checkVec3(L, 3, "ViewportFrame:setLight");
        }

        float intensity = 1.0f;
        if (!lua_isnoneornil(L, 4)) {
            intensity = check<float>(L, 4, "ViewportFrame:setLight");
        }

        auto settings = self->renderSettings();
        settings.lightDirection = direction;
        settings.lightColor = color;
        settings.lightIntensity = intensity;
        self->setRenderSettings(settings);
        return 0;
    }

    int viewportSetAmbient(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setAmbient");
        float const ambient = std::clamp(check<float>(L, 2, "ViewportFrame:setAmbient"), 0.0f, 4.0f);

        auto settings = self->renderSettings();
        settings.ambient = ambient;
        self->setRenderSettings(settings);
        return 0;
    }

    int viewportGetLight(lua_State* L) {
        auto const* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:getLight");
        RenderSettings const& settings = self->renderSettings();

        lua_createtable(L, 0, 4);
        pushVec3(L, settings.lightDirection);
        lua_setfield(L, -2, "direction");
        pushVec3(L, settings.lightColor);
        lua_setfield(L, -2, "color");
        push(L, settings.lightIntensity);
        lua_setfield(L, -2, "intensity");
        push(L, settings.ambient);
        lua_setfield(L, -2, "ambient");
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

    int viewportSetInstancePrimitiveMaterial(lua_State* L) {
        auto* self =
            Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setInstancePrimitiveMaterial");
        int const instanceId = check<int>(L, 2, "ViewportFrame:setInstancePrimitiveMaterial");
        int const primitiveIndex = check<int>(L, 3, "ViewportFrame:setInstancePrimitiveMaterial");

        std::shared_ptr<Material> material{};
        if (!lua_isnoneornil(L, 4)) {
            material = requireMaterial(L, 4, "ViewportFrame:setInstancePrimitiveMaterial");
        }

        push(L, self->setInstancePrimitiveMaterial(instanceId, primitiveIndex, std::move(material)));
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

    int viewportGetInstanceTransform(lua_State* L) {
        auto const* self =
            Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:getInstanceTransform");
        int const instanceId = check<int>(L, 2, "ViewportFrame:getInstanceTransform");

        auto const& instances = self->instances();
        auto const it = instances.find(instanceId);
        if (it == instances.end()) {
            lua_pushnil(L);
            return 1;
        }

        pushTransform(L, it->second.transform);
        return 1;
    }

    int viewportGetInstanceColor(lua_State* L) {
        auto const* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:getInstanceColor");
        int const instanceId = check<int>(L, 2, "ViewportFrame:getInstanceColor");

        auto const& instances = self->instances();
        auto const it = instances.find(instanceId);
        if (it == instances.end()) {
            lua_pushnil(L);
            return 1;
        }

        pushVec3(L, it->second.color);
        return 1;
    }

    int viewportGetInstanceMaterial(lua_State* L) {
        auto const* self =
            Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:getInstanceMaterial");
        int const instanceId = check<int>(L, 2, "ViewportFrame:getInstanceMaterial");

        auto const& instances = self->instances();
        auto const it = instances.find(instanceId);
        if (it == instances.end() || !it->second.materialOverride) {
            lua_pushnil(L);
            return 1;
        }

        pushMaterial(L, it->second.materialOverride);
        return 1;
    }

    int viewportGetInstancePrimitiveMaterial(lua_State* L) {
        auto const* self =
            Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:getInstancePrimitiveMaterial");
        int const instanceId = check<int>(L, 2, "ViewportFrame:getInstancePrimitiveMaterial");
        int const primitiveIndex = check<int>(L, 3, "ViewportFrame:getInstancePrimitiveMaterial");

        auto const& instances = self->instances();
        auto const it = instances.find(instanceId);
        if (it == instances.end()) {
            lua_pushnil(L);
            return 1;
        }

        auto const primIt = it->second.primitiveOverrides.find(primitiveIndex);
        if (primIt == it->second.primitiveOverrides.end() || !primIt->second) {
            lua_pushnil(L);
            return 1;
        }

        pushMaterial(L, primIt->second);
        return 1;
    }

    int viewportGetInstanceIds(lua_State* L) {
        auto const* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:getInstanceIds");
        auto const& instances = self->instances();

        lua_createtable(L, static_cast<int>(instances.size()), 0);
        int arrayIndex = 1;
        for (auto const& [id, _] : instances) {
            push(L, id);
            lua_rawseti(L, -2, arrayIndex++);
        }
        return 1;
    }

    int viewportInstanceCount(lua_State* L) {
        auto const* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:instanceCount");
        push(L, static_cast<int>(self->instances().size()));
        return 1;
    }

    int viewportSetCompositeEnabled(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setCompositeEnabled");
        self->setCompositeEnabled(check<bool>(L, 2, "ViewportFrame:setCompositeEnabled"));
        return 0;
    }

    int viewportTexture(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:texture");
        pushTextureHandle(L, self->ensureViewportTextureId());
        return 1;
    }

    int viewportAddDebugLine(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:addDebugLine");
        auto const from = checkVec3(L, 2, "ViewportFrame:addDebugLine");
        auto const to = checkVec3(L, 3, "ViewportFrame:addDebugLine");
        auto const color = checkVec3(L, 4, "ViewportFrame:addDebugLine");

        push(L, self->addDebugLine(from, to, color));
        return 1;
    }

    int viewportRemoveDebugLine(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:removeDebugLine");
        int const lineId = check<int>(L, 2, "ViewportFrame:removeDebugLine");

        push(L, self->removeDebugLine(lineId));
        return 1;
    }

    int viewportClearDebugLines(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:clearDebugLines");
        self->clearDebugLines();
        return 0;
    }

    int viewportSetDebugBounds(lua_State* L) {
        auto* self = Usertype<CCViewportFrame>::check(L, 1, "ViewportFrame:setDebugBounds");
        self->setDebugBounds(check<bool>(L, 2, "ViewportFrame:setDebugBounds"));
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
        Usertype<CCViewportFrame>::method(L, "setBackgroundColor", &viewportSetBackgroundColor);
        Usertype<CCViewportFrame>::method(L, "getBackgroundColor", &viewportGetBackgroundColor);
        Usertype<CCViewportFrame>::method(L, "setLight", &viewportSetLight);
        Usertype<CCViewportFrame>::method(L, "setAmbient", &viewportSetAmbient);
        Usertype<CCViewportFrame>::method(L, "getLight", &viewportGetLight);
        Usertype<CCViewportFrame>::method(L, "addMesh", &viewportAddMesh);
        Usertype<CCViewportFrame>::method(L, "setInstanceMaterial", &viewportSetInstanceMaterial);
        Usertype<CCViewportFrame>::method(
            L, "setInstancePrimitiveMaterial", &viewportSetInstancePrimitiveMaterial
        );
        Usertype<CCViewportFrame>::method(L, "setInstanceColor", &viewportSetInstanceColor);
        Usertype<CCViewportFrame>::method(L, "setInstanceTransform", &viewportSetInstanceTransform);
        Usertype<CCViewportFrame>::method(L, "getInstanceTransform", &viewportGetInstanceTransform);
        Usertype<CCViewportFrame>::method(L, "getInstanceColor", &viewportGetInstanceColor);
        Usertype<CCViewportFrame>::method(L, "getInstanceMaterial", &viewportGetInstanceMaterial);
        Usertype<CCViewportFrame>::method(
            L, "getInstancePrimitiveMaterial", &viewportGetInstancePrimitiveMaterial
        );
        Usertype<CCViewportFrame>::method(L, "getInstanceIds", &viewportGetInstanceIds);
        Usertype<CCViewportFrame>::method(L, "instanceCount", &viewportInstanceCount);
        Usertype<CCViewportFrame>::method(L, "removeInstance", &viewportRemoveInstance);
        Usertype<CCViewportFrame>::method(L, "clearInstances", &viewportClearInstances);
        Usertype<CCViewportFrame>::method(L, "setCompositeEnabled", &viewportSetCompositeEnabled);
        Usertype<CCViewportFrame>::method(L, "texture", &viewportTexture);
        Usertype<CCViewportFrame>::method(L, "addDebugLine", &viewportAddDebugLine);
        Usertype<CCViewportFrame>::method(L, "removeDebugLine", &viewportRemoveDebugLine);
        Usertype<CCViewportFrame>::method(L, "clearDebugLines", &viewportClearDebugLines);
        Usertype<CCViewportFrame>::method(L, "setDebugBounds", &viewportSetDebugBounds);

        getOrCreateTable(L, "gd3d.ViewportFrame");
        setTableCFunction(L, -1, "new", &viewportNew);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(gd3d_viewport_frame_lib, registerViewportFrame)
#endif
