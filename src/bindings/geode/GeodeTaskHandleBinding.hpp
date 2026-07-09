#pragma once

#include "framework/callback/LuaCallback.hpp"

#include <arc/future/Context.hpp>
#include <arc/task/Task.hpp>
#include <lua.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace luax {
    using GeodeTaskHandlePushValueFn = void (*)(lua_State* L, void const* value);

    class GeodeTaskHandleStateBase {
    public:
        enum class Status {
            Pending,
            Done,
            Error,
            Detached,
        };

        explicit GeodeTaskHandleStateBase(GeodeTaskHandlePushValueFn pushValue);
        virtual ~GeodeTaskHandleStateBase() = default;

        bool isPending() const;
        bool isDone() const;
        bool isDetached() const;
        bool hasCallbacks() const;

        void addCallback(std::shared_ptr<LuaCallback> cb);
        void cancel();
        void detach();
        void fireCallbacks();

        virtual void poll(arc::Context& cx) = 0;
        virtual void abortNative() = 0;
        virtual void detachNative() noexcept = 0;

    protected:
        void completeSuccess();
        void completeError(std::string error);
        GeodeTaskHandlePushValueFn pushValueFn() const;
        virtual void dropStoredResult() = 0;
        virtual void pushSuccessArgs(lua_State* L) const = 0;

    private:
        void fireCallback(std::shared_ptr<LuaCallback> const& cb);
        void pushCallbackArgs(lua_State* L) const;

        GeodeTaskHandlePushValueFn m_pushValue = nullptr;
        Status m_status = Status::Pending;
        std::string m_error;
        std::vector<std::shared_ptr<LuaCallback>> m_callbacks;
        bool m_firingCallbacks = false;
        bool m_detachAfterCallbacks = false;
    };

    template <class T>
    class GeodeTaskHandleState final : public GeodeTaskHandleStateBase {
    public:
        GeodeTaskHandleState(arc::TaskHandle<T> handle, GeodeTaskHandlePushValueFn pushValue) :
            GeodeTaskHandleStateBase(pushValue), m_handle(std::move(handle)) {}

        void poll(arc::Context& cx) override {
            if (!isPending()) return;
            try {
                auto result = m_handle.poll(cx);
                if (result) {
                    m_value.emplace(std::move(*result));
                    completeSuccess();
                }
            }
            catch (std::exception const& e) {
                detachNative();
                completeError(e.what());
            }
            catch (...) {
                detachNative();
                completeError("Task failed with an unknown exception");
            }
        }

        void abortNative() override {
            m_handle.abort();
        }

        void detachNative() noexcept override {
            m_handle.detach();
        }

    protected:
        void dropStoredResult() override {
            m_value.reset();
        }

        void pushSuccessArgs(lua_State* L) const override {
            if (m_value) {
                pushStoredValue(L, &*m_value);
            }
            else {
                lua_pushnil(L);
            }
            lua_pushnil(L);
        }

    private:
        void pushStoredValue(lua_State* L, void const* value) const {
            auto pushValue = pushValueFn();
            if (pushValue) {
                pushValue(L, value);
            }
            else {
                lua_pushnil(L);
            }
        }

        arc::TaskHandle<T> m_handle;
        std::optional<T> m_value;
    };

    template <>
    class GeodeTaskHandleState<void> final : public GeodeTaskHandleStateBase {
    public:
        GeodeTaskHandleState(arc::TaskHandle<void> handle, GeodeTaskHandlePushValueFn pushValue) :
            GeodeTaskHandleStateBase(pushValue), m_handle(std::move(handle)) {}

        void poll(arc::Context& cx) override {
            if (!isPending()) return;
            try {
                if (m_handle.poll(cx)) {
                    completeSuccess();
                }
            }
            catch (std::exception const& e) {
                detachNative();
                completeError(e.what());
            }
            catch (...) {
                detachNative();
                completeError("Task failed with an unknown exception");
            }
        }

        void abortNative() override {
            m_handle.abort();
        }

        void detachNative() noexcept override {
            m_handle.detach();
        }

    protected:
        void dropStoredResult() override {}

        void pushSuccessArgs(lua_State* L) const override {
            lua_pushnil(L);
            lua_pushnil(L);
        }

    private:
        arc::TaskHandle<void> m_handle;
    };

    void registerGeodeTaskHandleMetatable(lua_State* L);
    void pushGeodeTaskHandleState(lua_State* L, std::shared_ptr<GeodeTaskHandleStateBase> state);
    void pollGeodeTaskHandles(lua_State* L);
    void clearGeodeTaskHandles();
    void compactGeodeTaskHandles();

    template <class T>
    void pushGeodeTaskHandle(
        lua_State* L, arc::TaskHandle<T> handle, GeodeTaskHandlePushValueFn pushValue
    ) {
        auto state = std::make_shared<GeodeTaskHandleState<T>>(std::move(handle), pushValue);
        pushGeodeTaskHandleState(L, std::move(state));
    }

    template <class T>
    void pushOptionalGeodeTaskHandle(
        lua_State* L, std::optional<arc::TaskHandle<T>> handle, GeodeTaskHandlePushValueFn pushValue
    ) {
        if (!handle) {
            lua_pushnil(L);
            return;
        }
        pushGeodeTaskHandle<T>(L, std::move(*handle), pushValue);
    }
} // namespace luax
