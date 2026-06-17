#pragma once

#include "core/Runtime.hpp"
#include "framework/BindingHost.hpp"
#include "framework/usertype/LuaRef.hpp"

#include <Geode/loader/Log.hpp>
#include <functional>
#include <lua.h>
#include <memory>
#include <string_view>

namespace luax {
    inline void logCallbackFailure(std::string_view context) {
        geode::log::warn("[lua:{}] callback failed", context);
    }

    class LuaCallback {
    public:
        using PushArgsFn = void (*)(lua_State* L, void* ctx);
        using PopResultsFn = void (*)(lua_State* L, void* ctx);

        LuaCallback() = default;

        LuaCallback(lua_State* L, int index) {
            reset(L, index);
        }

        void reset(lua_State* L, int index) {
            m_ref = std::make_shared<LuaRef>(L, index);
        }

        void reset() {
            m_ref.reset();
        }

        bool valid() const {
            return m_ref && m_ref->valid();
        }

        bool invoke(
            int nargs, int nresults, std::string_view context, int deadlineMs,
            PushArgsFn pushArgs = nullptr, void* pushCtx = nullptr,
            PopResultsFn popResults = nullptr, void* popCtx = nullptr
        ) const {
            if (Runtime::isShuttingDown()) return false;
            auto* host = BindingHost::getIfInitialized();
            if (!host || !host->ready()) return false;
            auto* L = host->state();
            if (!L || !m_ref) return false;

            int top = lua_gettop(L);
            if (!m_ref->push()) return false;
            if (pushArgs) {
                pushArgs(L, pushCtx);
            }
            BindingHost::ResourcesRootScope scope(*host, m_ref->resourcesRoot());
            auto result = host->protectedCall(L, nargs, nresults, context, deadlineMs);
            if (result.isOk() && popResults && nresults > 0) {
                popResults(L, popCtx);
            }
            lua_settop(L, top);
            return result.isOk();
        }

        std::shared_ptr<LuaRef> const& ref() const {
            return m_ref;
        }

    private:
        std::shared_ptr<LuaRef> m_ref;
    };
} // namespace luax
