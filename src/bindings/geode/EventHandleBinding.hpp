#pragma once

#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/lifecycle/Lifecycle.hpp"
#include "framework/stack/TaggedMetatable.hpp"

#include <Geode/loader/Event.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <new>
#include <optional>
#include <utility>

namespace luax::events {
    template <
        char const* MetaName, typename Data, void (*PushFn)(lua_State*, Data const&),
        void (*ReadFn)(lua_State*, int, char const*, Data&)>
    struct EventHandleBinding {
        using State = geode::ListenerHandle;

        struct Box {
            std::shared_ptr<State> state;
        };

        static WeakHandlePool<State>& activeListeners() {
            static WeakHandlePool<State> listeners;
            return listeners;
        }

        static bool& shutdownHookRegistered() {
            static bool registered = false;
            return registered;
        }

        static void clearState() {
            activeListeners().clearAll([](State& listener) {
                listener = {};
            });
            shutdownHookRegistered() = false;
        }

        static void ensureShutdownHookRegistered() {
            luax::ensureShutdownHook(shutdownHookRegistered(), &clearState);
        }

        static void rememberListener(std::shared_ptr<State> const& state) {
            activeListeners().track(state);
            activeListeners().compactAndCountLive();
            ensureShutdownHookRegistered();
        }

        static void pushListener(lua_State* L, std::shared_ptr<State> state) {
            auto* box = static_cast<Box*>(lua_newuserdata(L, sizeof(Box)));
            new (box) Box{std::move(state)};
            luaL_getmetatable(L, MetaName);
            lua_setmetatable(L, -2);
        }

        static Box* checkListener(lua_State* L, int idx) {
            return static_cast<Box*>(luaL_checkudata(L, idx, MetaName));
        }

        static void registerListenerMetatable(lua_State* L) {
            luaL_Reg methods[] = {
                {"disconnect", listenerDisconnect},
                {nullptr, nullptr},
            };
            registerTaggedMetatable(L, MetaName, std::nullopt, methods, &listenerGc);
        }

        static bool invoke(std::shared_ptr<LuaCallback> const& cb, char const* context, Data& data) {
            if (!cb || !cb->valid()) return false;

            struct Ctx {
                Data* data;
                char const* context;
                int dataRef = LUA_NOREF;
                bool stop = false;
            } ctx{&data, context, LUA_NOREF, false};

            bool ok = cb->invoke(
                1,
                1,
                context,
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    PushFn(L, *c->data);
                    lua_pushvalue(L, -1);
                    c->dataRef = lua_ref(L, -1);
                    lua_pop(L, 1);
                },
                &ctx,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    c->stop = lua_toboolean(L, -1) != 0;
                    if (c->dataRef == LUA_NOREF || c->dataRef == LUA_REFNIL) return;
                    lua_getref(L, c->dataRef);
                    lua_pushlightuserdata(L, const_cast<char*>(c->context));
                    lua_pushlightuserdata(L, c->data);
                    lua_pushcclosurek(L, readThunk, "eventDataRead", 2, nullptr);
                    lua_pushvalue(L, -4);
                    if (lua_pcall(L, 1, 0, 0) != 0) {
                        char const* msg = lua_tostring(L, -1);
                        geode::log::warn(
                            "[lua:{}] event data read failed: {}", c->context, msg ? msg : "(unknown error)"
                        );
                        lua_pop(L, 1);
                    }
                    lua_pop(L, 1);
                },
                &ctx
            );

            auto* runtime = Runtime::getIfInitialized();
            if (ctx.dataRef != LUA_NOREF && ctx.dataRef != LUA_REFNIL && runtime && runtime->state()) {
                lua_unref(runtime->state(), ctx.dataRef);
            }
            if (!ok) {
                logCallbackFailure(context);
            }
            return ok && ctx.stop;
        }

    private:
        static int readThunk(lua_State* L) {
            auto* context = static_cast<char const*>(lua_touserdata(L, lua_upvalueindex(1)));
            auto* data = static_cast<Data*>(lua_touserdata(L, lua_upvalueindex(2)));
            ReadFn(L, 1, context, *data);
            return 0;
        }

        static int listenerGc(lua_State* L) {
            auto* box = checkListener(L, 1);
            box->~Box();
            return 0;
        }

        static int listenerDisconnect(lua_State* L) {
            auto* box = checkListener(L, 1);
            if (box->state) *box->state = {};
            return 0;
        }
    };
} // namespace luax::events
