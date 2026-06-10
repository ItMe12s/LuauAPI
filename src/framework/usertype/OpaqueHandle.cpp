#include "framework/usertype/OpaqueHandle.hpp"

#include "framework/Binding.hpp"

#include <lualib.h>
#include <new>

namespace luax {
    namespace {
        constexpr char const* kOpaqueHandleMetatable = "luax:OpaqueHandle";
    } // namespace

    namespace detail {
        bool isOpaqueHandle(lua_State* L, int idx) {
            if (lua_type(L, idx) != LUA_TUSERDATA) {
                return false;
            }
            return lua_userdatatag(L, idx) == opaqueHandleTag();
        }

        void ensureOpaqueHandleMetatable(lua_State* L) {
            if (luaL_newmetatable(L, kOpaqueHandleMetatable)) {
                lua_pushstring(L, "locked");
                lua_setfield(L, -2, "__metatable");
                lua_pushstring(L, "OpaqueHandle");
                lua_setfield(L, -2, "__type");
            }
            lua_pop(L, 1);
        }
    } // namespace detail

    geode::Result<void> registerOpaqueHandle(lua_State* L) {
        detail::ensureOpaqueHandleMetatable(L);
        lua_getuserdatametatable(L, detail::opaqueHandleTag());
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return geode::Ok();
        }
        lua_pop(L, 1);

        luaL_getmetatable(L, kOpaqueHandleMetatable);
        lua_setuserdatametatable(L, detail::opaqueHandleTag());
        return geode::Ok();
    }

    void pushOpaqueHandle(lua_State* L, void* ptr) {
        if (ptr == nullptr) {
            lua_pushnil(L);
            return;
        }
        detail::ensureOpaqueHandleMetatable(L);
        auto* storage = lua_newuserdatataggedwithmetatable(
            L, sizeof(OpaqueHandleBlock), detail::opaqueHandleTag()
        );
        new (storage) OpaqueHandleBlock{ptr};
    }

    void* checkOpaqueHandle(lua_State* L, int idx, char const* label) {
        idx = lua_absindex(L, idx);
        if (!detail::isOpaqueHandle(L, idx)) {
            luaL_error(L, "%s expected opaque handle at arg %d", label, idx);
        }
        auto* block = static_cast<OpaqueHandleBlock*>(lua_touserdata(L, idx));
        return block ? block->ptr : nullptr;
    }

    void* tryOpaqueHandle(lua_State* L, int idx) {
        if (!detail::isOpaqueHandle(L, idx)) {
            return nullptr;
        }
        auto* block = static_cast<OpaqueHandleBlock*>(lua_touserdata(L, idx));
        return block ? block->ptr : nullptr;
    }
} // namespace luax

LUAX_BINDING_PRIORITY(opaque_handle, luax::registerOpaqueHandle, 0)
