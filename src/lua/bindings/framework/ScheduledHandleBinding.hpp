#pragma once

#include <concepts>
#include <cstdint>
#include <lua.h>
#include <lualib.h>

namespace luax {
    struct ScheduledHandle {
        std::uint64_t id = 0;
    };

    template <class Traits>
    concept ScheduledHandleTraits = requires {
        { Traits::kMeta } -> std::convertible_to<char const*>;
        { Traits::kTypeName } -> std::convertible_to<char const*>;
        { Traits::userdataTag() } -> std::same_as<int>;
        typename Traits::Scheduler;
    } && requires(typename Traits::Scheduler& scheduler, std::uint64_t id) {
        { Traits::Scheduler::get() } -> std::same_as<typename Traits::Scheduler&>;
        { scheduler.cancel(id) } -> std::same_as<void>;
    };

    template <ScheduledHandleTraits Traits>
    struct ScheduledHandleBinding {
        static void cancel(ScheduledHandle* handle) {
            if (handle && handle->id != 0) {
                Traits::Scheduler::get().cancel(handle->id);
                handle->id = 0;
            }
        }

        static void dtor(lua_State* L, void* ud) {
            (void)L;
            cancel(static_cast<ScheduledHandle*>(ud));
        }

        static void push(lua_State* L, std::uint64_t id) {
            auto* handle = static_cast<ScheduledHandle*>(
                lua_newuserdatataggedwithmetatable(L, sizeof(ScheduledHandle), Traits::userdataTag())
            );
            handle->id = id;
        }

        static ScheduledHandle* check(lua_State* L, int idx) {
            return static_cast<ScheduledHandle*>(luaL_checkudata(L, idx, Traits::kMeta));
        }

        static int luaCancel(lua_State* L) {
            cancel(check(L, 1));
            return 0;
        }

        static int luaGc(lua_State* L) {
            cancel(check(L, 1));
            return 0;
        }

        static void registerMetatable(lua_State* L) {
            if (luaL_newmetatable(L, Traits::kMeta)) {
                lua_pushcfunction(L, &luaCancel, "cancel");
                lua_setfield(L, -2, "cancel");
                lua_pushcfunction(L, &luaGc, "__gc");
                lua_setfield(L, -2, "__gc");
                lua_pushvalue(L, -1);
                lua_setfield(L, -2, "__index");
                lua_pushstring(L, "locked");
                lua_setfield(L, -2, "__metatable");
                lua_pushstring(L, Traits::kTypeName);
                lua_setfield(L, -2, "__type");
            }
            lua_pop(L, 1);

            lua_getuserdatametatable(L, Traits::userdataTag());
            if (!lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return;
            }
            lua_pop(L, 1);

            luaL_getmetatable(L, Traits::kMeta);
            lua_setuserdatametatable(L, Traits::userdataTag());
            lua_setuserdatadtor(L, Traits::userdataTag(), &dtor);
        }
    };
} // namespace luax
