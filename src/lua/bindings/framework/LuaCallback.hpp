#pragma once

#include "lua/bindings/framework/BindingHost.hpp"
#include "lua/bindings/framework/LuaRef.hpp"

#include <lua.h>

#include <functional>
#include <memory>
#include <string_view>

namespace luax {
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
            int nargs,
            int nresults,
            std::string_view context,
            int deadlineMs,
            PushArgsFn pushArgs = nullptr,
            void* pushCtx = nullptr,
            PopResultsFn popResults = nullptr,
            void* popCtx = nullptr
        ) const {
            auto* host = BindingHost::getIfInitialized();
            if (!host || !host->ready()) return false;
            auto* L = host->state();
            if (!L || !m_ref || !m_ref->push()) return false;

            int top = lua_gettop(L);
            if (pushArgs) {
                pushArgs(L, pushCtx);
            }
            BindingHost::ResourcesRootScope scope(*host, m_ref->resourcesRoot());
            bool ok = host->protectedCall(nargs, nresults, context, deadlineMs);
            if (ok && popResults && nresults > 0) {
                popResults(L, popCtx);
            }
            lua_settop(L, top);
            return ok;
        }

        std::shared_ptr<LuaRef> const& ref() const {
            return m_ref;
        }

    private:
        std::shared_ptr<LuaRef> m_ref;
    };
}
