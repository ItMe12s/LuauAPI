#pragma once

#include "framework/callback/LuaCallback.hpp"

#include <cocos2d.h>
#include <memory>

namespace luax {
    class LuaMenuHandler final : public cocos2d::CCObject {
    public:
        static LuaMenuHandler* create(lua_State* L, int fnIndex);

        void onCallback(cocos2d::CCObject* sender);

    private:
        std::shared_ptr<LuaCallback> m_callback;
    };
} // namespace luax
