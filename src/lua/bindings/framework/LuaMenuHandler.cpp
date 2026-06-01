#include "LuaMenuHandler.hpp"

#include "Usertype.hpp"
#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/Geode.hpp>

#include <unordered_map>
#include <vector>

namespace luax {
    namespace {
        std::vector<LuaMenuHandler*>& orphanHandlers() {
            static auto* value = new std::vector<LuaMenuHandler*>();
            return *value;
        }

        std::unordered_map<cocos2d::CCObject*, std::vector<LuaMenuHandler*>>& anchorMap() {
            static auto* value =
                new std::unordered_map<cocos2d::CCObject*, std::vector<LuaMenuHandler*>>();
            return *value;
        }

        bool g_shutdownHookRegistered = false;
        bool g_orphanCapWarned = false;
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
        if (!handler) return;
        if (!anchor) {
            registerOrphanMenuHandler(handler);
            return;
        }
        handler->retain();
        anchorMap()[anchor].push_back(handler);
        ensureMenuHandlerShutdownHook();
    }

    void evictMenuHandlersIfFinalRelease(cocos2d::CCObject* anchor) {
        if (!anchor || anchor->retainCount() > 1) return;
        auto& map = anchorMap();
        auto it = map.find(anchor);
        if (it == map.end()) return;
        for (auto* handler : it->second) {
            handler->release();
        }
        map.erase(it);
    }

    void registerOrphanMenuHandler(LuaMenuHandler* handler) {
        if (!handler) return;
        if (orphanHandlers().size() >= kMaxCallbackTrampolines && !g_orphanCapWarned) {
            g_orphanCapWarned = true;
            geode::log::warn(
                "orphan menu handler registry exceeded soft cap ({})",
                kMaxCallbackTrampolines
            );
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
        for (auto& [_, handlers] : anchorMap()) {
            for (auto* handler : handlers) {
                handler->release();
            }
        }
        anchorMap().clear();
    }

    void ensureMenuHandlerShutdownHook() {
        if (g_shutdownHookRegistered) return;
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return;
        runtime->registerShutdownHook(&clearOrphanMenuHandlers);
        g_shutdownHookRegistered = true;
    }
}
