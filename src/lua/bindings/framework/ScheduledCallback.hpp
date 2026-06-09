#pragma once

#include "lua/bindings/framework/LuaRef.hpp"
#include "lua/runtime/Runtime.hpp"

#include <string_view>

namespace luax {
    inline bool fireProtectedCallback(LuaRef& callback, std::string_view context, int deadlineMs) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return false;
        auto* L = callback.luaState();
        if (!L) return false;
        int top = lua_gettop(L);
        if (!callback.push()) return false;
        Runtime::ResourcesRootScope scope(*runtime, callback.resourcesRoot());
        bool ok = runtime->protectedCall(0, 0, context, deadlineMs).isOk();
        lua_settop(L, top);
        return ok;
    }
} // namespace luax
