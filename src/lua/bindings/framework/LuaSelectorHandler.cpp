#include "LuaSelectorHandler.hpp"

#include "Usertype.hpp"
#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

namespace luax {
    LuaScheduleHandler* LuaScheduleHandler::create(lua_State* L, int fnIndex) {
        luaL_checktype(L, fnIndex, LUA_TFUNCTION);
        auto* handler = new LuaScheduleHandler();
        handler->m_callback = std::make_shared<LuaCallback>(L, fnIndex);
        handler->autorelease();
        return handler;
    }

    void LuaScheduleHandler::onSchedule(float dt) {
        if (!m_callback || !m_callback->valid()) return;
        struct Ctx { float dt; };
        Ctx ctx{dt};
        m_callback->invoke(
            1, 0, "schedule callback", kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(c->dt));
            },
            &ctx
        );
    }

    LuaCallFuncHandler* LuaCallFuncHandler::create(lua_State* L, int fnIndex) {
        luaL_checktype(L, fnIndex, LUA_TFUNCTION);
        auto* handler = new LuaCallFuncHandler();
        handler->m_callback = std::make_shared<LuaCallback>(L, fnIndex);
        handler->autorelease();
        return handler;
    }

    void LuaCallFuncHandler::onCallFunc() {
        if (!m_callback || !m_callback->valid()) return;
        m_callback->invoke(0, 0, "callfunc callback", kHookScriptDeadlineMs);
    }

    LuaCallFuncNHandler* LuaCallFuncNHandler::create(lua_State* L, int fnIndex) {
        luaL_checktype(L, fnIndex, LUA_TFUNCTION);
        auto* handler = new LuaCallFuncNHandler();
        handler->m_callback = std::make_shared<LuaCallback>(L, fnIndex);
        handler->autorelease();
        return handler;
    }

    void LuaCallFuncNHandler::onCallFuncN(cocos2d::CCNode* node) {
        if (!m_callback || !m_callback->valid()) return;
        struct Ctx { cocos2d::CCNode* node; };
        Ctx ctx{node};
        m_callback->invoke(
            1, 0, "callfuncn callback", kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                Usertype<cocos2d::CCNode>::pushBorrowed(L, c->node);
            },
            &ctx
        );
    }

    LuaCallFuncNDHandler* LuaCallFuncNDHandler::create(lua_State* L, int fnIndex) {
        luaL_checktype(L, fnIndex, LUA_TFUNCTION);
        auto* handler = new LuaCallFuncNDHandler();
        handler->m_callback = std::make_shared<LuaCallback>(L, fnIndex);
        handler->autorelease();
        return handler;
    }

    void LuaCallFuncNDHandler::onCallFuncND(cocos2d::CCNode* node, void* data) {
        if (!m_callback || !m_callback->valid()) return;
        struct Ctx { cocos2d::CCNode* node; void* data; };
        Ctx ctx{node, data};
        m_callback->invoke(
            2, 0, "callfuncnd callback", kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                Usertype<cocos2d::CCNode>::pushBorrowed(L, c->node);
                if (c->data == nullptr) lua_pushnil(L);
                else lua_pushlightuserdata(L, c->data);
            },
            &ctx
        );
    }

    LuaCallFuncOHandler* LuaCallFuncOHandler::create(lua_State* L, int fnIndex) {
        luaL_checktype(L, fnIndex, LUA_TFUNCTION);
        auto* handler = new LuaCallFuncOHandler();
        handler->m_callback = std::make_shared<LuaCallback>(L, fnIndex);
        handler->autorelease();
        return handler;
    }

    void LuaCallFuncOHandler::onCallFuncO(cocos2d::CCObject* obj) {
        if (!m_callback || !m_callback->valid()) return;
        struct Ctx { cocos2d::CCObject* obj; };
        Ctx ctx{obj};
        m_callback->invoke(
            1, 0, "callfunco callback", kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                Usertype<cocos2d::CCObject>::pushBorrowed(L, c->obj);
            },
            &ctx
        );
    }
}
