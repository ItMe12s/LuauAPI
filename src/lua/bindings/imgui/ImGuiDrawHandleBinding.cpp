#include "ImGuiDrawHandleBinding.hpp"

#include "ImGuiDrawScheduler.hpp"
#include "ImGuiHost.hpp"
#include "lua/Config.hpp"
#include "lua/bindings/framework/LuaRef.hpp"
#include "lua/bindings/framework/ScheduledHandleBinding.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/framework/UserdataTags.hpp"

#include <cstdint>
#include <lua.h>
#include <lualib.h>

namespace luax {
    namespace {
        struct ImGuiDrawHandleTraits {
            using Scheduler = ImGuiDrawScheduler;
            static constexpr char const* kMeta = "luax.ImGuiDrawHandle";
            static constexpr char const* kTypeName = "ImGuiDrawHandle";

            static constexpr int userdataTag() noexcept {
                return detail::imguiDrawHandleTag();
            }
        };

        using ImGuiDrawHandleBinding = ScheduledHandleBinding<ImGuiDrawHandleTraits>;

        void pushHandle(lua_State* L, std::uint64_t id) {
            ImGuiDrawHandleBinding::push(L, id);
        }
    } // namespace

    int imguiCancel(lua_State* L) {
        return ImGuiDrawHandleBinding::luaCancel(L);
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
        ImGuiDrawHandleBinding::registerMetatable(L);
    }

    geode::Result<void> registerImGuiDrawHandle(lua_State* L) {
        registerImGuiDrawHandleMetatable(L);

        lua_newtable(L);
        setTableCFunction(L, -1, "onDraw", &imguiOnDraw);
        lua_setglobal(L, "imgui");

        return geode::Ok();
    }
} // namespace luax
