#include "bindings/render3d/Gd3dShared.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/UserdataTags.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;
    using namespace luax::gd3d;

    void materialHandleDtor(lua_State*, void* ud) {
        static_cast<MaterialBox*>(ud)->~MaterialBox();
    }

    void registerMaterialMetatable(lua_State* L) {
        if (luaL_newmetatable(L, kMaterialMeta)) {
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            lua_pushstring(L, kMaterialTypeName);
            lua_setfield(L, -2, "__type");
        }
        lua_pop(L, 1);

        lua_getuserdatametatable(L, detail::materialTag());
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);

        luaL_getmetatable(L, kMaterialMeta);
        lua_setuserdatametatable(L, detail::materialTag());
        lua_setuserdatadtor(L, detail::materialTag(), &materialHandleDtor);
    }
} // namespace

namespace luax {
    geode::Result<void> registerMaterial(lua_State* L) {
        registerMaterialMetatable(L);
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(gd3d_material_lib, registerMaterial)
#endif
