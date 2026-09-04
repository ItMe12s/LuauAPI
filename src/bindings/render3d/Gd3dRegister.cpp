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
        geode::Result<void> (*const registrars[])(lua_State*) = {
            &registerTransform,
#if !defined(LUAUAPI_HOST_TESTS)
            &registerMaterial,
            &registerTexture,
#endif
            &registerGltf,
            &registerProceduralMesh,
#if !defined(LUAUAPI_HOST_TESTS)
            &registerViewportFrame,
#endif
        };

        for (auto* registrar : registrars) {
            if (auto result = registrar(L); result.isErr()) {
                return result;
            }
        }

        return geode::Ok();
    }
} // namespace luax

LUAX_BINDING(gd3d_lib, luax::registerGd3d)
