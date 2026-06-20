#pragma once

#include <Geode/Result.hpp>

struct lua_State;

namespace luax {
    int imguiOnDraw(lua_State* L);
    int imguiCancel(lua_State* L);

    void registerImGuiDrawHandleMetatable(lua_State* L);
} // namespace luax
