#pragma once

#include "lua/bindings/framework/LuaCallback.hpp"

#include <cocos2d.h>

#include <memory>

namespace luax {
    class LuaMenuHandler : public cocos2d::CCObject {
    public:
        static LuaMenuHandler* create(lua_State* L, int fnIndex);

        void onCallback(cocos2d::CCObject* sender);

    private:
        std::shared_ptr<LuaCallback> m_callback;
    };

    void anchorMenuHandler(cocos2d::CCObject* anchor, LuaMenuHandler* handler);
    void registerOrphanMenuHandler(LuaMenuHandler* handler);
    void clearOrphanMenuHandlers();
    void ensureMenuHandlerShutdownHook();
}
