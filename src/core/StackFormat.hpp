#pragma once

#include "require/PathSandbox.hpp"

#include <Geode/utils/general.hpp>
#include <filesystem>
#include <lua.h>
#include <string>
#include <string_view>

struct lua_State;

namespace luax {

    std::string formatDebugSource(char const* source, std::filesystem::path const& resourcesRoot);
    std::string redactHostPaths(std::string_view text, std::filesystem::path const& resourcesRoot);
    std::string formatLuaStack(lua_State* L, std::filesystem::path const& resourcesRoot);
    std::string formatPendingCall(lua_State* L, int funcIndex, std::filesystem::path const& resourcesRoot);
    bool hasStackFrames(std::string_view luaStack);

} // namespace luax
