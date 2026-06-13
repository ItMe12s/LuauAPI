#pragma once

#include "framework/stack/TableUtil.hpp"

#include <lua.h>
#include <lualib.h>
#include <optional>

namespace luax {
    inline void registerTaggedMetatable(
        lua_State* L, char const* meta, std::optional<int> tag, luaL_Reg const* methods,
        std::optional<lua_CFunction> gc = std::nullopt,
        std::optional<lua_Destructor> dtor = std::nullopt,
        std::optional<char const*> typeName = std::nullopt
    ) {
        if (luaL_newmetatable(L, meta)) {
            for (luaL_Reg const* reg = methods; reg->name != nullptr; ++reg) {
                setTableCFunction(L, -1, reg->name, reg->func);
            }
            if (gc) {
                lua_pushcfunction(L, *gc, "__gc");
                lua_setfield(L, -2, "__gc");
            }
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            if (typeName) {
                lua_pushstring(L, *typeName);
                lua_setfield(L, -2, "__type");
            }
        }
        lua_pop(L, 1);

        if (!tag) {
            return;
        }

        lua_getuserdatametatable(L, *tag);
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);

        luaL_getmetatable(L, meta);
        lua_setuserdatametatable(L, *tag);
        if (dtor) {
            lua_setuserdatadtor(L, *tag, *dtor);
        }
    }
} // namespace luax
