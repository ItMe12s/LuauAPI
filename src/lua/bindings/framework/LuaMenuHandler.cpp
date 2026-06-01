#include "LuaMenuHandler.hpp"

#include "Usertype.hpp"
#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

#include <vector>

namespace luax {
    namespace {
        std::vector<LuaMenuHandler*>& orphanHandlers() {
            static auto* value = new std::vector<LuaMenuHandler*>();
            return *value;
        }

        bool g_shutdownHookRegistered = false;
    }

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
        m_callback->invoke(
            1,
            0,
            "menu callback",
            kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                Usertype<cocos2d::CCObject>::pushBorrowed(L, c->sender);
            },
            &ctx
        );
    }

    void anchorMenuHandler(cocos2d::CCObject* anchor, LuaMenuHandler* handler) {
        if (!anchor || !handler) return;
        registerOrphanMenuHandler(handler);
    }

    void registerOrphanMenuHandler(LuaMenuHandler* handler) {
        if (!handler) return;
        if (orphanHandlers().size() >= kMaxCallbackTrampolines) {
            return;
        }
        handler->retain();
        orphanHandlers().push_back(handler);
        ensureMenuHandlerShutdownHook();
    }

    void clearOrphanMenuHandlers() {
        for (auto* handler : orphanHandlers()) {
            handler->release();
        }
        orphanHandlers().clear();
    }

    void ensureMenuHandlerShutdownHook() {
        if (g_shutdownHookRegistered) return;
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return;
        runtime->registerShutdownHook(&clearOrphanMenuHandlers);
        g_shutdownHookRegistered = true;
    }
}
