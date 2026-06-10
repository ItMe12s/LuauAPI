#include "lua/bindings/framework/LuaMenuHandler.hpp"

#include "lua/Config.hpp"
#include "lua/bindings/framework/LuaTrampolineRegistry.hpp"
#include "lua/bindings/framework/Usertype.hpp"
#include "lua/runtime/Runtime.hpp"

namespace luax {
    LuaMenuHandler* LuaMenuHandler::create(lua_State* L, int fnIndex) {
        luaL_checktype(L, fnIndex, LUA_TFUNCTION);
        auto* handler = new LuaMenuHandler();
        handler->m_callback = std::make_shared<LuaCallback>(L, fnIndex);
        handler->autorelease();
        return handler;
    }

    void LuaMenuHandler::onCallback(cocos2d::CCObject* sender) {
        if (!m_callback || !m_callback->valid()) return;

        struct Ctx {
            cocos2d::CCObject* sender;
        };

        Ctx ctx{sender};
        if (!m_callback->invoke(
                1,
                0,
                "menu callback",
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    Usertype<cocos2d::CCObject>::pushBorrowed(L, c->sender);
                },
                &ctx
            )) {
            logCallbackFailure("menu callback");
        }
    }

    void anchorMenuHandler(cocos2d::CCObject* anchor, LuaMenuHandler* handler) {
        anchorTrampoline(anchor, handler);
    }

    void registerOrphanMenuHandler(LuaMenuHandler* handler) {
        registerOrphanTrampoline(handler);
    }

    void evictMenuHandlersIfFinalRelease(cocos2d::CCObject* anchor) {
        evictTrampolinesIfFinalRelease(anchor);
    }

    void clearOrphanMenuHandlers() {
        clearOrphanTrampolines();
    }

    void ensureMenuHandlerShutdownHook() {
        ensureTrampolineShutdownHook();
    }
} // namespace luax
