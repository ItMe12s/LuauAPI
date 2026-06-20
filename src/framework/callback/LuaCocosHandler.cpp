#include "framework/callback/LuaCocosHandler.hpp"

#include "core/Config.hpp"
#include "framework/usertype/OpaqueHandle.hpp"
#include "framework/usertype/Usertype.hpp"

namespace luax {
    namespace detail {
        bool invokeCocosCallback(
            std::shared_ptr<LuaCallback>& callback, int nargs, std::string_view context,
            LuaCallback::PushArgsFn pushArgs, void* pushCtx
        ) {
            if (!callback || !callback->valid()) {
                return false;
            }
            if (!callback->invoke(nargs, 0, context, kHookScriptDeadlineMs, pushArgs, pushCtx)) {
                logCallbackFailure(context);
                return false;
            }
            return true;
        }
    } // namespace detail

    LuaMenuHandler* LuaMenuHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaMenuHandler>(L, fnIndex);
    }

    void LuaMenuHandler::onCallback(cocos2d::CCObject* sender) {
        struct Ctx {
            cocos2d::CCObject* sender;
        };

        Ctx ctx{sender};
        detail::invokeCocosCallback(
            m_callback,
            1,
            "menu callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                Usertype<cocos2d::CCObject>::pushBorrowed(L, c->sender);
            },
            &ctx
        );
    }

    LuaScheduleHandler* LuaScheduleHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaScheduleHandler>(L, fnIndex);
    }

    void LuaScheduleHandler::onSchedule(float dt) {
        struct Ctx {
            float dt;
        };

        Ctx ctx{dt};
        detail::invokeCocosCallback(
            m_callback,
            1,
            "schedule callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                lua_pushnumber(L, static_cast<double>(c->dt));
            },
            &ctx
        );
    }

    LuaCallFuncHandler* LuaCallFuncHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaCallFuncHandler>(L, fnIndex);
    }

    void LuaCallFuncHandler::onCallFunc() {
        detail::invokeCocosCallback(m_callback, 0, "callfunc callback", nullptr, nullptr);
    }

    LuaCallFuncNHandler* LuaCallFuncNHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaCallFuncNHandler>(L, fnIndex);
    }

    void LuaCallFuncNHandler::onCallFuncN(cocos2d::CCNode* node) {
        struct Ctx {
            cocos2d::CCNode* node;
        };

        Ctx ctx{node};
        detail::invokeCocosCallback(
            m_callback,
            1,
            "callfuncn callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                Usertype<cocos2d::CCNode>::pushBorrowed(L, c->node);
            },
            &ctx
        );
    }

    LuaCallFuncNDHandler* LuaCallFuncNDHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaCallFuncNDHandler>(L, fnIndex);
    }

    void LuaCallFuncNDHandler::onCallFuncND(cocos2d::CCNode* node, void* data) {
        struct Ctx {
            cocos2d::CCNode* node;
            void* data;
        };

        Ctx ctx{node, data};
        detail::invokeCocosCallback(
            m_callback,
            2,
            "callfuncnd callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                Usertype<cocos2d::CCNode>::pushBorrowed(L, c->node);
                if (c->data == nullptr) lua_pushnil(L);
                else pushOpaqueHandle(L, c->data);
            },
            &ctx
        );
    }

    LuaCallFuncOHandler* LuaCallFuncOHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaCallFuncOHandler>(L, fnIndex);
    }

    void LuaCallFuncOHandler::onCallFuncO(cocos2d::CCObject* obj) {
        struct Ctx {
            cocos2d::CCObject* obj;
        };

        Ctx ctx{obj};
        detail::invokeCocosCallback(
            m_callback,
            1,
            "callfunco callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                Usertype<cocos2d::CCObject>::pushBorrowed(L, c->obj);
            },
            &ctx
        );
    }
} // namespace luax
