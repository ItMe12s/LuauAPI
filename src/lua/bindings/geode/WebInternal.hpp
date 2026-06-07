#pragma once

#include "lua/bindings/framework/LuaCallback.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <algorithm>
#include <cstddef>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <new>
#include <vector>

namespace luax {
    namespace web = geode::utils::web;

    inline constexpr char const* kRequestMeta = "luax.WebRequest";
    inline constexpr char const* kResponseMeta = "luax.WebResponse";
    inline constexpr char const* kHandleMeta = "luax.WebHandle";
    inline constexpr char const* kMultipartMeta = "luax.MultipartForm";
    inline constexpr char const* kListenerMeta = "luax.WebListenerHandle";

    struct WebRequestBox {
        web::WebRequest request;
        std::shared_ptr<LuaCallback> progress;
    };

    struct WebResponseBox {
        web::WebResponse response;
    };

    struct MultipartBox {
        web::MultipartForm form;
    };

    struct WebTask {
        geode::async::TaskHolder<web::WebResponse> holder;
        std::size_t id = 0;
        bool done = false;

        explicit WebTask(std::size_t taskID) : id(taskID) {}

        ~WebTask() {
            cancel();
        }

        void cancel() {
            if (done) return;
            holder.cancel();
            done = true;
        }
    };

    struct WebHandleBox {
        std::shared_ptr<WebTask> task;
    };

    struct WebListenerState {
        geode::ListenerHandle handle;
        bool connected = true;

        ~WebListenerState() {
            disconnect();
        }

        void disconnect() {
            if (!connected) return;
            handle = geode::ListenerHandle();
            connected = false;
        }
    };

    struct WebListenerBox {
        std::shared_ptr<WebListenerState> state;
    };

    inline std::vector<std::weak_ptr<WebTask>>& activeTasks() {
        static auto* tasks = new std::vector<std::weak_ptr<WebTask>>();
        return *tasks;
    }

    inline std::vector<std::weak_ptr<WebListenerState>>& activeListeners() {
        static auto* listeners = new std::vector<std::weak_ptr<WebListenerState>>();
        return *listeners;
    }

    inline bool s_shutdownHookRegistered = false;

    inline void clearWebState() {
        for (auto& weak : activeTasks()) {
            if (auto task = weak.lock()) {
                task->cancel();
            }
        }
        activeTasks().clear();
        for (auto& weak : activeListeners()) {
            if (auto listener = weak.lock()) {
                listener->disconnect();
            }
        }
        activeListeners().clear();
        s_shutdownHookRegistered = false;
    }

    inline void ensureShutdownHook() {
        if (s_shutdownHookRegistered) return;
        auto* rt = Runtime::getIfInitialized();
        if (!rt) return;
        rt->registerShutdownHook(&clearWebState);
        s_shutdownHookRegistered = true;
    }

    inline void compactWeakState() {
        auto& tasks = activeTasks();
        tasks.erase(
            std::remove_if(
                tasks.begin(),
                tasks.end(),
                [](auto const& weak) {
                    return weak.expired();
                }
            ),
            tasks.end()
        );
        auto& listeners = activeListeners();
        listeners.erase(
            std::remove_if(
                listeners.begin(),
                listeners.end(),
                [](auto const& weak) {
                    return weak.expired();
                }
            ),
            listeners.end()
        );
    }

    inline WebRequestBox* checkRequestBox(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<WebRequestBox*>(luaL_checkudata(L, idx, kRequestMeta));
        if (!box) luaL_error(L, "%s expected WebRequest at arg %d", method, idx);
        return box;
    }

    inline web::WebRequest& checkRequest(lua_State* L, int idx, char const* method) {
        return checkRequestBox(L, idx, method)->request;
    }

    inline web::WebResponse& checkResponse(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<WebResponseBox*>(luaL_checkudata(L, idx, kResponseMeta));
        if (!box) luaL_error(L, "%s expected WebResponse at arg %d", method, idx);
        return box->response;
    }

    inline MultipartBox* checkMultipartBox(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<MultipartBox*>(luaL_checkudata(L, idx, kMultipartMeta));
        if (!box) luaL_error(L, "%s expected MultipartForm at arg %d", method, idx);
        return box;
    }

    inline WebHandleBox* checkHandle(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<WebHandleBox*>(luaL_checkudata(L, idx, kHandleMeta));
        if (!box) luaL_error(L, "%s expected WebHandle at arg %d", method, idx);
        return box;
    }

    inline WebListenerBox* checkListener(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<WebListenerBox*>(luaL_checkudata(L, idx, kListenerMeta));
        if (!box) luaL_error(L, "%s expected WebListenerHandle at arg %d", method, idx);
        return box;
    }

    inline void pushRequest(lua_State* L, web::WebRequest request) {
        auto* box = static_cast<WebRequestBox*>(lua_newuserdata(L, sizeof(WebRequestBox)));
        new (box) WebRequestBox{std::move(request), nullptr};
        luaL_getmetatable(L, kRequestMeta);
        lua_setmetatable(L, -2);
    }

    inline void pushResponse(lua_State* L, web::WebResponse response) {
        auto* box = static_cast<WebResponseBox*>(lua_newuserdata(L, sizeof(WebResponseBox)));
        new (box) WebResponseBox{std::move(response)};
        luaL_getmetatable(L, kResponseMeta);
        lua_setmetatable(L, -2);
    }

    inline void pushMultipart(lua_State* L, web::MultipartForm form = web::MultipartForm()) {
        auto* box = static_cast<MultipartBox*>(lua_newuserdata(L, sizeof(MultipartBox)));
        new (box) MultipartBox{std::move(form)};
        luaL_getmetatable(L, kMultipartMeta);
        lua_setmetatable(L, -2);
    }

    inline void pushHandle(lua_State* L, std::shared_ptr<WebTask> task) {
        auto* box = static_cast<WebHandleBox*>(lua_newuserdata(L, sizeof(WebHandleBox)));
        new (box) WebHandleBox{std::move(task)};
        luaL_getmetatable(L, kHandleMeta);
        lua_setmetatable(L, -2);
    }

    inline void pushListener(lua_State* L, std::shared_ptr<WebListenerState> state) {
        auto* box = static_cast<WebListenerBox*>(lua_newuserdata(L, sizeof(WebListenerBox)));
        new (box) WebListenerBox{std::move(state)};
        luaL_getmetatable(L, kListenerMeta);
        lua_setmetatable(L, -2);
    }

    inline int requestGc(lua_State* L) {
        auto* box = static_cast<WebRequestBox*>(luaL_checkudata(L, 1, kRequestMeta));
        box->~WebRequestBox();
        return 0;
    }

    inline int responseGc(lua_State* L) {
        auto* box = static_cast<WebResponseBox*>(luaL_checkudata(L, 1, kResponseMeta));
        box->~WebResponseBox();
        return 0;
    }

    inline int multipartGc(lua_State* L) {
        auto* box = static_cast<MultipartBox*>(luaL_checkudata(L, 1, kMultipartMeta));
        box->~MultipartBox();
        return 0;
    }

    inline int handleGc(lua_State* L) {
        auto* box = static_cast<WebHandleBox*>(luaL_checkudata(L, 1, kHandleMeta));
        if (box->task) box->task->cancel();
        box->~WebHandleBox();
        return 0;
    }

    inline int listenerGc(lua_State* L) {
        auto* box = static_cast<WebListenerBox*>(luaL_checkudata(L, 1, kListenerMeta));
        box->~WebListenerBox();
        return 0;
    }
} // namespace luax
