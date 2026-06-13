#pragma once

#include "framework/stack/TaggedMetatable.hpp"

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
            luaL_Reg const methods[] = {
                {"cancel", &luaCancel},
                {nullptr, nullptr},
            };
            registerTaggedMetatable(
                L, Traits::kMeta, Traits::userdataTag(), methods, &luaGc, &dtor, Traits::kTypeName
            );
        }
    };
} // namespace luax
