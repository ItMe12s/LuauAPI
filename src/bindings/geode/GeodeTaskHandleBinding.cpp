#include "bindings/geode/GeodeTaskHandleBinding.hpp"

#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaCallback.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <algorithm>
#include <arc/task/Waker.hpp>
#include <lua.h>
#include <lualib.h>
#include <new>
#include <utility>
#include <vector>

namespace luax {
    namespace {
        constexpr char const* kMeta = "luax.GeodeTaskHandle";
        constexpr char const* kTypeName = "GeodeTaskHandle";

        struct GeodeTaskHandleBox {
            std::shared_ptr<GeodeTaskHandleStateBase> state;
        };

        std::vector<std::shared_ptr<GeodeTaskHandleStateBase>>& activeHandles() {
            static std::vector<std::shared_ptr<GeodeTaskHandleStateBase>> s_handles;
            return s_handles;
        }

        GeodeTaskHandleBox* checkBox(lua_State* L, int idx, char const* label) {
            auto* box = static_cast<GeodeTaskHandleBox*>(luaL_checkudata(L, idx, kMeta));
            if (!box || !box->state) {
                luaL_error(L, "%s expected GeodeTaskHandle at arg %d", label, idx);
            }
            return box;
        }

        int luaOnComplete(lua_State* L) {
            auto* box = checkBox(L, 1, "GeodeTaskHandle:onComplete");
            luaL_checktype(L, 2, LUA_TFUNCTION);
            box->state->addCallback(std::make_shared<LuaCallback>(L, 2));
            lua_pushvalue(L, 1);
            return 1;
        }

        int luaCancel(lua_State* L) {
            auto* box = checkBox(L, 1, "GeodeTaskHandle:cancel");
            box->state->cancel();
            compactGeodeTaskHandles();
            return 0;
        }

        int luaDetach(lua_State* L) {
            auto* box = checkBox(L, 1, "GeodeTaskHandle:detach");
            box->state->detach();
            compactGeodeTaskHandles();
            return 0;
        }

        int luaIsPending(lua_State* L) {
            auto* box = checkBox(L, 1, "GeodeTaskHandle:isPending");
            lua_pushboolean(L, box->state->isPending());
            return 1;
        }

        int luaIsDone(lua_State* L) {
            auto* box = checkBox(L, 1, "GeodeTaskHandle:isDone");
            lua_pushboolean(L, box->state->isDone());
            return 1;
        }

        int luaIsDetached(lua_State* L) {
            auto* box = checkBox(L, 1, "GeodeTaskHandle:isDetached");
            lua_pushboolean(L, box->state->isDetached());
            return 1;
        }

        int luaGc(lua_State* L) {
            auto* box = static_cast<GeodeTaskHandleBox*>(luaL_checkudata(L, 1, kMeta));
            if (box && box->state && !box->state->hasCallbacks()) {
                box->state->detach();
                compactGeodeTaskHandles();
            }
            if (box) {
                box->~GeodeTaskHandleBox();
            }
            return 0;
        }

        void activateHandle(lua_State* L, std::shared_ptr<GeodeTaskHandleStateBase> const& state) {
            auto& handles = activeHandles();
            compactGeodeTaskHandles();
            if (handles.size() >= kMaxGeodeTaskHandles) {
                if (state) {
                    state->detach();
                }
                luaL_error(
                    L,
                    "geode task handles: too many active handles (limit %d)",
                    static_cast<int>(kMaxGeodeTaskHandles)
                );
            }
            handles.push_back(state);
        }
    } // namespace

    GeodeTaskHandleStateBase::GeodeTaskHandleStateBase(GeodeTaskHandlePushValueFn pushValue) :
        m_pushValue(pushValue) {}

    bool GeodeTaskHandleStateBase::isPending() const {
        return m_status == Status::Pending;
    }

    bool GeodeTaskHandleStateBase::isDone() const {
        return m_status == Status::Done || m_status == Status::Error;
    }

    bool GeodeTaskHandleStateBase::isDetached() const {
        return m_status == Status::Detached || m_detachAfterCallbacks;
    }

    bool GeodeTaskHandleStateBase::hasCallbacks() const {
        return !m_callbacks.empty();
    }

    GeodeTaskHandlePushValueFn GeodeTaskHandleStateBase::pushValueFn() const {
        return m_pushValue;
    }

    void GeodeTaskHandleStateBase::addCallback(std::shared_ptr<LuaCallback> cb) {
        if (!cb) return;
        if (m_status == Status::Pending) {
            m_callbacks.push_back(std::move(cb));
            return;
        }
        if (m_detachAfterCallbacks) {
            return;
        }
        if (isDone()) {
            fireCallback(cb);
        }
    }

    void GeodeTaskHandleStateBase::cancel() {
        if (m_status == Status::Pending) {
            try {
                abortNative();
            }
            catch (...) {
            }
            m_callbacks.clear();
            dropStoredResult();
            m_status = Status::Detached;
            return;
        }
        m_callbacks.clear();
        if (m_firingCallbacks) {
            m_detachAfterCallbacks = true;
            return;
        }
        dropStoredResult();
        m_status = Status::Detached;
    }

    void GeodeTaskHandleStateBase::detach() {
        if (m_status == Status::Pending) {
            detachNative();
            m_callbacks.clear();
            dropStoredResult();
            m_status = Status::Detached;
            return;
        }
        m_callbacks.clear();
        if (m_firingCallbacks) {
            m_detachAfterCallbacks = true;
            return;
        }
        dropStoredResult();
        m_status = Status::Detached;
    }

    void GeodeTaskHandleStateBase::completeSuccess() {
        if (m_status != Status::Pending) return;
        m_status = Status::Done;
        fireCallbacks();
    }

    void GeodeTaskHandleStateBase::completeError(std::string error) {
        if (m_status != Status::Pending) return;
        m_error = std::move(error);
        dropStoredResult();
        m_status = Status::Error;
        fireCallbacks();
    }

    void GeodeTaskHandleStateBase::fireCallbacks() {
        auto callbacks = std::move(m_callbacks);
        m_callbacks.clear();
        m_firingCallbacks = true;
        for (auto const& cb : callbacks) {
            fireCallback(cb);
        }
        m_firingCallbacks = false;
        if (m_detachAfterCallbacks) {
            m_detachAfterCallbacks = false;
            m_callbacks.clear();
            dropStoredResult();
            m_status = Status::Detached;
        }
    }

    void GeodeTaskHandleStateBase::pushCallbackArgs(lua_State* L) const {
        if (m_status == Status::Error) {
            lua_pushnil(L);
            luax::push(L, m_error);
            return;
        }
        pushSuccessArgs(L);
    }

    void GeodeTaskHandleStateBase::fireCallback(std::shared_ptr<LuaCallback> const& cb) {
        if (!cb || !cb->valid() || Runtime::isShuttingDown()) return;

        struct Ctx {
            GeodeTaskHandleStateBase const* state;
        } ctx{this};

        if (!cb->invoke(
                2,
                0,
                "geode.TaskHandle.onComplete",
                kHookScriptDeadlineMs,
                +[](lua_State* L, void* raw) {
                    static_cast<Ctx*>(raw)->state->pushCallbackArgs(L);
                },
                &ctx
            )) {
            logCallbackFailure("geode.TaskHandle.onComplete");
        }
    }

    void registerGeodeTaskHandleMetatable(lua_State* L) {
        if (luaL_newmetatable(L, kMeta)) {
            lua_pushstring(L, kTypeName);
            lua_setfield(L, -2, "__type");

            lua_newtable(L);
            setTableCFunction(L, -1, "onComplete", &luaOnComplete);
            setTableCFunction(L, -1, "cancel", &luaCancel);
            setTableCFunction(L, -1, "detach", &luaDetach);
            setTableCFunction(L, -1, "isPending", &luaIsPending);
            setTableCFunction(L, -1, "isDone", &luaIsDone);
            setTableCFunction(L, -1, "isDetached", &luaIsDetached);
            lua_setfield(L, -2, "__index");

            lua_pushcfunction(L, &luaGc, "GeodeTaskHandle.__gc");
            lua_setfield(L, -2, "__gc");
        }
        lua_pop(L, 1);
    }

    void pushGeodeTaskHandleState(lua_State* L, std::shared_ptr<GeodeTaskHandleStateBase> state) {
        registerGeodeTaskHandleMetatable(L);
        activateHandle(L, state);
        auto* box = static_cast<GeodeTaskHandleBox*>(lua_newuserdata(L, sizeof(GeodeTaskHandleBox)));
        new (box) GeodeTaskHandleBox{std::move(state)};
        luaL_getmetatable(L, kMeta);
        lua_setmetatable(L, -2);
    }

    void pollGeodeTaskHandles(lua_State* L) {
        (void)L;
        auto snapshot = activeHandles();
        auto waker = arc::Waker::noop();
        arc::Context cx(&waker);
        for (auto const& state : snapshot) {
            if (state && state->isPending()) {
                state->poll(cx);
            }
        }
        compactGeodeTaskHandles();
    }

    void clearGeodeTaskHandles() {
        auto snapshot = activeHandles();
        for (auto const& state : snapshot) {
            if (state) {
                state->detach();
            }
        }
        activeHandles().clear();
    }

    void compactGeodeTaskHandles() {
        auto& handles = activeHandles();
        handles.erase(
            std::remove_if(
                handles.begin(),
                handles.end(),
                [](std::shared_ptr<GeodeTaskHandleStateBase> const& state) {
                    return !state || !state->isPending();
                }
            ),
            handles.end()
        );
    }

    geode::Result<void> registerGeodeTaskHandleBinding(lua_State* L) {
        registerGeodeTaskHandleMetatable(L);
        if (auto* runtime = static_cast<Runtime*>(lua_callbacks(L)->userdata)) {
            runtime->registerShutdownHook([] {
                clearGeodeTaskHandles();
            });
        }
        return geode::Ok();
    }
} // namespace luax

namespace {
    LUAX_BINDING(geode_task_handle, luax::registerGeodeTaskHandleBinding)
}
