#include "framework/callback/LuaCocosHandler.hpp"

#include "core/Config.hpp"
#include "framework/usertype/OpaqueHandle.hpp"
#include "framework/usertype/Usertype.hpp"

namespace luax {
    void LuaCocosHandlerBase::bindAnchor(cocos2d::CCObject* anchor) {
        m_anchor = geode::WeakRef<cocos2d::CCObject>(anchor);
        m_anchorBound = true;
    }

    bool LuaCocosHandlerBase::anchorAlive() const {
        return !m_anchorBound || m_anchor.valid();
    }

    namespace detail {
        struct ScopedRetain {
            cocos2d::CCObject* obj = nullptr;
            bool held = false;

            explicit ScopedRetain(cocos2d::CCObject* o) : obj(o) {
                if (obj) {
                    obj->retain();
                    held = true;
                }
            }

            ~ScopedRetain() {
                if (held && obj) obj->release();
            }
        };

        bool invokeCocosCallback(
            cocos2d::CCObject* handler, std::shared_ptr<LuaCallback>& callback, int nargs,
            std::string_view context, LuaCallback::PushArgsFn pushArgs, void* pushCtx,
            cocos2d::CCObject* argKeepalive
        ) {
            if (handler) {
                auto* cocosHandler = static_cast<LuaCocosHandlerBase*>(handler);
                if (!cocosHandler->anchorAlive()) {
                    return false;
                }
                if (handler->retainCount() < 1) {
                    return false;
                }
            }
            ScopedRetain keepHandler(handler);
            ScopedRetain keepArg(argKeepalive);
            auto cb = callback;
            if (!cb || !cb->valid()) {
                return false;
            }
            if (!cb->invoke(nargs, 0, context, kHookScriptDeadlineMs, pushArgs, pushCtx)) {
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
        if (!anchorAlive()) return;

        struct Ctx {
            cocos2d::CCObject* sender;
        };

        Ctx ctx{sender};
        detail::invokeCocosCallback(
            this,
            m_callback,
            1,
            "menu callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                detail::pushCallbackArg(L, c->sender);
            },
            &ctx,
            sender
        );
    }

    LuaScheduleHandler* LuaScheduleHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaScheduleHandler>(L, fnIndex);
    }

    void LuaScheduleHandler::onSchedule(float dt) {
        if (!anchorAlive()) return;

        struct Ctx {
            float dt;
        };

        Ctx ctx{dt};
        detail::invokeCocosCallback(
            this,
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
        if (!anchorAlive()) return;
        detail::invokeCocosCallback(this, m_callback, 0, "callfunc callback", nullptr, nullptr);
    }

    LuaCallFuncNHandler* LuaCallFuncNHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaCallFuncNHandler>(L, fnIndex);
    }

    void LuaCallFuncNHandler::onCallFuncN(cocos2d::CCNode* node) {
        if (!anchorAlive()) return;

        struct Ctx {
            cocos2d::CCNode* node;
        };

        Ctx ctx{node};
        detail::invokeCocosCallback(
            this,
            m_callback,
            1,
            "callfuncn callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                detail::pushCallbackArg(L, c->node);
            },
            &ctx,
            node
        );
    }

    LuaCallFuncNDHandler* LuaCallFuncNDHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaCallFuncNDHandler>(L, fnIndex);
    }

    void LuaCallFuncNDHandler::onCallFuncND(cocos2d::CCNode* node, void* data) {
        if (!anchorAlive()) return;

        struct Ctx {
            cocos2d::CCNode* node;
            void* data;
        };

        Ctx ctx{node, data};
        detail::invokeCocosCallback(
            this,
            m_callback,
            2,
            "callfuncnd callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                detail::pushCallbackArg(L, c->node);
                if (c->data == nullptr) lua_pushnil(L);
                else pushOpaqueHandle(L, c->data);
            },
            &ctx,
            node
        );
    }

    LuaCallFuncOHandler* LuaCallFuncOHandler::create(lua_State* L, int fnIndex) {
        return LuaCocosHandlerBase::create<LuaCallFuncOHandler>(L, fnIndex);
    }

    void LuaCallFuncOHandler::onCallFuncO(cocos2d::CCObject* obj) {
        if (!anchorAlive()) return;

        struct Ctx {
            cocos2d::CCObject* obj;
        };

        Ctx ctx{obj};
        detail::invokeCocosCallback(
            this,
            m_callback,
            1,
            "callfunco callback",
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                detail::pushCallbackArg(L, c->obj);
            },
            &ctx,
            obj
        );
    }
} // namespace luax
