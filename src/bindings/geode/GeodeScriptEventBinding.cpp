#include "ScriptEventInternal.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/lifecycle/Lifecycle.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"

#include <Geode/loader/Priority.hpp>
#include <ScriptEvents.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace {
    using namespace luax;

    constexpr char kScriptListenerMeta[] = "luax.ScriptEventListenerHandle";

    struct ScriptListenerState {
        geode::ListenerHandle handle;
    };

    struct ScriptListenerBox {
        std::shared_ptr<ScriptListenerState> state;
    };

    WeakHandlePool<ScriptListenerState>& activeScriptListeners() {
        static WeakHandlePool<ScriptListenerState> listeners;
        return listeners;
    }

    bool& scriptShutdownHookRegistered() {
        static bool registered = false;
        return registered;
    }

    void clearScriptListeners() {
        activeScriptListeners().clearAll([](ScriptListenerState& listener) {
            listener.handle = {};
        });
        scriptShutdownHookRegistered() = false;
    }

    void rememberScriptListener(std::shared_ptr<ScriptListenerState> const& state) {
        activeScriptListeners().track(state);
        activeScriptListeners().compactAndCountLive();
        ensureShutdownHook(scriptShutdownHookRegistered(), &clearScriptListeners);
    }

    bool invokeScriptListener(
        std::shared_ptr<LuaCallback> const& cb, char const* context, std::string_view topic,
        std::string_view payload
    ) {
        if (!cb || !cb->valid()) return false;

        struct Ctx {
            std::string_view topic;
            std::string_view payload;
            bool stop = false;
        } ctx{topic, payload, false};

        bool ok = cb->invoke(
            2,
            1,
            context,
            kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                lua_pushlstring(L, c->topic.data(), c->topic.size());
                lua_pushlstring(L, c->payload.data(), c->payload.size());
            },
            &ctx,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                c->stop = lua_toboolean(L, -1) != 0;
            },
            &ctx
        );
        if (!ok) {
            logCallbackFailure(context);
        }
        return ok && ctx.stop;
    }

    int optPriority(lua_State* L, int idx) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return geode::Priority::Normal;
        return check<int>(L, idx, "geode.ScriptEvent listener");
    }

    void pushScriptListener(lua_State* L, std::shared_ptr<ScriptListenerState> state) {
        auto* box = static_cast<ScriptListenerBox*>(lua_newuserdata(L, sizeof(ScriptListenerBox)));
        new (box) ScriptListenerBox{std::move(state)};
        luaL_getmetatable(L, kScriptListenerMeta);
        lua_setmetatable(L, -2);
    }

    int scriptListenerGc(lua_State* L) {
        auto* box = static_cast<ScriptListenerBox*>(luaL_checkudata(L, 1, kScriptListenerMeta));
        box->~ScriptListenerBox();
        return 0;
    }

    int scriptListenerDisconnect(lua_State* L) {
        auto* box = static_cast<ScriptListenerBox*>(luaL_checkudata(L, 1, kScriptListenerMeta));
        if (box->state) {
            box->state->handle = {};
        }
        return 0;
    }

    void registerScriptListenerMetatable(lua_State* L) {
        luaL_Reg methods[] = {
            {"disconnect", scriptListenerDisconnect},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(L, kScriptListenerMeta, std::nullopt, methods, &scriptListenerGc);
    }

    int scriptEventPost(lua_State* L) {
        std::string topic = check<std::string>(L, 1, "geode.ScriptEvent.post");
        std::string payload;
        if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
            payload = check<std::string>(L, 2, "geode.ScriptEvent.post");
        }
        imes::luauapi::LuaScriptEvent().send(topic, payload);
        return 0;
    }

    int scriptEventListener(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 1);
        int priority = optPriority(L, 2);
        auto state = std::make_shared<ScriptListenerState>(ScriptLuaListen().listen(
            [cb](std::string_view topic, std::string_view payload) {
                return invokeScriptListener(cb, "geode.ScriptEvent.listen", topic, payload);
            },
            priority
        ));
        rememberScriptListener(state);
        pushScriptListener(L, std::move(state));
        return 1;
    }

    int scriptEventListenerFor(lua_State* L) {
        auto topic = check<std::string>(L, 1, "geode.ScriptEvent.listenFor");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 2);
        int priority = optPriority(L, 3);
        auto state = std::make_shared<ScriptListenerState>(ScriptLuaListenFor(topic).listen(
            [cb](std::string_view t, std::string_view p) {
                return invokeScriptListener(cb, "geode.ScriptEvent.listenFor", t, p);
            },
            priority
        ));
        rememberScriptListener(state);
        pushScriptListener(L, std::move(state));
        return 1;
    }

    geode::Result<void> registerScriptEvent(lua_State* L) {
        registerScriptListenerMetatable(L);
        getOrCreateTable(L, "geode.ScriptEvent");
        setTableCFunction(L, -1, "post", &scriptEventPost);
        setTableCFunction(L, -1, "listen", &scriptEventListener);
        setTableCFunction(L, -1, "listenFor", &scriptEventListenerFor);
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace

namespace luax {
    geode::Result<void> registerGeodeScriptEvent(lua_State* L) {
        return registerScriptEvent(L);
    }
} // namespace luax

LUAX_BINDING(geode_script_event_lib, registerGeodeScriptEvent)