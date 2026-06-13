#include "framework/Binding.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>

namespace luax {
    geode::Result<void> registerTransform(lua_State* L);
    geode::Result<void> registerMaterial(lua_State* L);
    geode::Result<void> registerTexture(lua_State* L);
    geode::Result<void> registerGltf(lua_State* L);
    geode::Result<void> registerProceduralMesh(lua_State* L);
    geode::Result<void> registerViewportFrame(lua_State* L);

    geode::Result<void> registerGd3d(lua_State* L) {
        auto result = registerTransform(L);
        if (result.isErr()) {
            return result;
        }

#if !defined(LUAUAPI_HOST_TESTS)
        result = registerMaterial(L);
        if (result.isErr()) {
            return result;
        }

        result = registerTexture(L);
        if (result.isErr()) {
            return result;
        }

        result = registerGltf(L);
        if (result.isErr()) {
            return result;
        }

        result = registerProceduralMesh(L);
        if (result.isErr()) {
            return result;
        }

        result = registerViewportFrame(L);
        if (result.isErr()) {
            return result;
        }
#endif

        return geode::Ok();
    }
} // namespace luax

LUAX_BINDING(gd3d_lib, luax::registerGd3d)
