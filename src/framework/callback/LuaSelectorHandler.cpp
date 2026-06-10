#include "framework/callback/LuaSelectorHandler.hpp"

#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/usertype/OpaqueHandle.hpp"
#include "framework/usertype/Usertype.hpp"

#define LUAX_SELECTOR_CREATE(HandlerType)                                \
    HandlerType* HandlerType::create(lua_State* L, int fnIndex) {        \
        luaL_checktype(L, fnIndex, LUA_TFUNCTION);                       \
        auto* handler = new HandlerType();                               \
        handler->m_callback = std::make_shared<LuaCallback>(L, fnIndex); \
        handler->autorelease();                                          \
        return handler;                                                  \
    }

namespace luax {
    LUAX_SELECTOR_CREATE(LuaScheduleHandler)

    void LuaScheduleHandler::onSchedule(float dt) {
        if (!m_callback || !m_callback->valid()) return;

        struct Ctx {
            float dt;
        };

        Ctx ctx{dt};
        if (!m_callback->invoke(
                1,
                0,
                "schedule callback",
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    lua_pushnumber(L, static_cast<double>(c->dt));
                },
                &ctx
            )) {
            logCallbackFailure("schedule callback");
        }
    }

    LUAX_SELECTOR_CREATE(LuaCallFuncHandler)

    void LuaCallFuncHandler::onCallFunc() {
        if (!m_callback || !m_callback->valid()) return;
        if (!m_callback->invoke(0, 0, "callfunc callback", kHookScriptDeadlineMs)) {
            logCallbackFailure("callfunc callback");
        }
    }

    LUAX_SELECTOR_CREATE(LuaCallFuncNHandler)

    void LuaCallFuncNHandler::onCallFuncN(cocos2d::CCNode* node) {
        if (!m_callback || !m_callback->valid()) return;

        struct Ctx {
            cocos2d::CCNode* node;
        };

        Ctx ctx{node};
        if (!m_callback->invoke(
                1,
                0,
                "callfuncn callback",
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    Usertype<cocos2d::CCNode>::pushBorrowed(L, c->node);
                },
                &ctx
            )) {
            logCallbackFailure("callfuncn callback");
        }
    }

    LUAX_SELECTOR_CREATE(LuaCallFuncNDHandler)

    void LuaCallFuncNDHandler::onCallFuncND(cocos2d::CCNode* node, void* data) {
        if (!m_callback || !m_callback->valid()) return;

        struct Ctx {
            cocos2d::CCNode* node;
            void* data;
        };

        Ctx ctx{node, data};
        if (!m_callback->invoke(
                2,
                0,
                "callfuncnd callback",
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    Usertype<cocos2d::CCNode>::pushBorrowed(L, c->node);
                    if (c->data == nullptr) lua_pushnil(L);
                    else pushOpaqueHandle(L, c->data);
                },
                &ctx
            )) {
            logCallbackFailure("callfuncnd callback");
        }
    }

    LUAX_SELECTOR_CREATE(LuaCallFuncOHandler)

    void LuaCallFuncOHandler::onCallFuncO(cocos2d::CCObject* obj) {
        if (!m_callback || !m_callback->valid()) return;

        struct Ctx {
            cocos2d::CCObject* obj;
        };

        Ctx ctx{obj};
        if (!m_callback->invoke(
                1,
                0,
                "callfunco callback",
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    auto* c = static_cast<Ctx*>(raw);
                    Usertype<cocos2d::CCObject>::pushBorrowed(L, c->obj);
                },
                &ctx
            )) {
            logCallbackFailure("callfunco callback");
        }
    }
} // namespace luax

#undef LUAX_SELECTOR_CREATE
