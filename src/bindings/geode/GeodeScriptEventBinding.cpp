#include "EventHandleBinding.hpp"
#include "ScriptEventInternal.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/UserdataTags.hpp"

#include <ScriptEvents.hpp>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {
    using namespace luax;

    constexpr char kScriptListenerMeta[] = "luax.ScriptEventListenerHandle";

    using ScriptListenerBinding =
        events::ListenerHandleBase<kScriptListenerMeta, detail::scriptListenerTag()>;

    bool invokeScriptListener(
        std::shared_ptr<LuaCallback> const& cb, char const* context, std::string_view topic,
        std::string_view payload
    ) {
        struct Ctx : CallbackStopFlag {
            std::string_view topic;
            std::string_view payload;
        } ctx;

        ctx.topic = topic;
        ctx.payload = payload;

        return invoke2ArgCallback(
            cb,
            context,
            ctx,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                lua_pushlstring(L, c->topic.data() ? c->topic.data() : "", c->topic.size());
                lua_pushlstring(L, c->payload.data() ? c->payload.data() : "", c->payload.size());
            },
            &ctx
        );
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
        return ScriptListenerBinding::registerListener(
            L, 1, 2, "geode.ScriptEvent.listen", [](std::shared_ptr<LuaCallback> const& cb, int priority) {
                return LuaListenerEvent().listen(
                    [cb](std::string_view topic, std::string_view payload) {
                        return invokeScriptListener(cb, "geode.ScriptEvent.listen", topic, payload);
                    },
                    priority
                );
            }
        );
    }

    int scriptEventListenerFor(lua_State* L) {
        auto topic = check<std::string>(L, 1, "geode.ScriptEvent.listenFor");
        return ScriptListenerBinding::registerListener(
            L,
            2,
            3,
            "geode.ScriptEvent.listenFor",
            [topic](std::shared_ptr<LuaCallback> const& cb, int priority) {
                return LuaListenerForEvent(topic).listen(
                    [cb](std::string_view t, std::string_view p) {
                        return invokeScriptListener(cb, "geode.ScriptEvent.listenFor", t, p);
                    },
                    priority
                );
            }
        );
    }

    geode::Result<void> registerScriptEvent(lua_State* L) {
        ScriptListenerBinding::registerListenerMetatable(L);
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
