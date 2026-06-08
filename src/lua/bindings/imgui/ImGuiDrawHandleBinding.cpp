#include "ImGuiDrawHandleBinding.hpp"

#include "ImGuiDrawScheduler.hpp"
#include "ImGuiHost.hpp"
#include "lua/Config.hpp"
#include "lua/bindings/framework/LuaRef.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/framework/UserdataTags.hpp"

#include <cstdint>
#include <lua.h>
#include <lualib.h>

namespace luax {
    namespace {
        constexpr char const* kHandleMeta = "luax.ImGuiDrawHandle";

        struct ImGuiDrawHandle {
            std::uint64_t id;
        };

        void cancelHandle(ImGuiDrawHandle* handle) {
            if (handle && handle->id != 0) {
                ImGuiDrawScheduler::get().cancel(handle->id);
                handle->id = 0;
            }
        }

        void imguiHandleDtor(lua_State* L, void* ud) {
            (void)L;
            cancelHandle(static_cast<ImGuiDrawHandle*>(ud));
        }

        void pushHandle(lua_State* L, std::uint64_t id) {
            auto* handle = static_cast<ImGuiDrawHandle*>(lua_newuserdatataggedwithmetatable(
                L, sizeof(ImGuiDrawHandle), detail::imguiDrawHandleTag()
            ));
            handle->id = id;
        }

        int imguiHandleGc(lua_State* L) {
            auto* handle = static_cast<ImGuiDrawHandle*>(luaL_checkudata(L, 1, kHandleMeta));
            cancelHandle(handle);
            return 0;
        }
    } // namespace

    int imguiCancel(lua_State* L) {
        auto* handle = static_cast<ImGuiDrawHandle*>(luaL_checkudata(L, 1, kHandleMeta));
        cancelHandle(handle);
        return 0;
    }

    int imguiOnDraw(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        if (ImGuiDrawScheduler::get().full()) {
            luaL_error(
                L,
                "imgui.onDraw: too many draw callbacks (limit %d)",
                static_cast<int>(kMaxImGuiDrawCallbacks)
            );
        }
        LuaRef ref;
        ref.reset(L, 1);
        std::uint64_t id = ImGuiDrawScheduler::get().add(std::move(ref));
#if !defined(LUAUAPI_HOST_TESTS)
        initImGuiHost();
#endif
        pushHandle(L, id);
        return 1;
    }

    void registerImGuiDrawHandleMetatable(lua_State* L) {
        if (luaL_newmetatable(L, kHandleMeta)) {
            lua_pushcfunction(L, &imguiCancel, "cancel");
            lua_setfield(L, -2, "cancel");
            lua_pushcfunction(L, &imguiHandleGc, "__gc");
            lua_setfield(L, -2, "__gc");
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            lua_pushstring(L, "ImGuiDrawHandle");
            lua_setfield(L, -2, "__type");
        }
        lua_pop(L, 1);

        lua_getuserdatametatable(L, detail::imguiDrawHandleTag());
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return;
        }
        lua_pop(L, 1);

        luaL_getmetatable(L, kHandleMeta);
        lua_setuserdatametatable(L, detail::imguiDrawHandleTag());
        lua_setuserdatadtor(L, detail::imguiDrawHandleTag(), &imguiHandleDtor);
    }

    geode::Result<void> registerImGuiDrawHandle(lua_State* L) {
        registerImGuiDrawHandleMetatable(L);

        lua_newtable(L);
        setTableCFunction(L, -1, "onDraw", &imguiOnDraw);
        lua_setglobal(L, "imgui");

        return geode::Ok();
    }
} // namespace luax
