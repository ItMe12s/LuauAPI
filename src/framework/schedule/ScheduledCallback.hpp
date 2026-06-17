#pragma once

#include "core/Runtime.hpp"
#include "framework/usertype/LuaRef.hpp"

#include <string_view>
#include <thread>

namespace luax {
    inline bool fireProtectedCallback(LuaRef& callback, std::string_view context, int deadlineMs) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return false;
        auto* L = callback.luaState();
        if (!L) return false;
        int top = lua_gettop(L);
        if (!callback.push()) return false;
#if defined(GEODE_IS_MACOS)
        if (!Runtime::isMainThread()) {
            Runtime::setMainThreadId(std::this_thread::get_id());
        }
#endif
        Runtime::ResourcesRootScope scope(*runtime, callback.resourcesRoot());
        bool ok = runtime->protectedCall(L, 0, 0, context, deadlineMs).isOk();
        lua_settop(L, top);
        return ok;
    }
} // namespace luax
