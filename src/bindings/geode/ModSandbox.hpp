#pragma once

#include <filesystem>
#include <optional>

struct lua_State;

namespace luax {
    struct SandboxTarget {
        std::filesystem::path path;
        bool writable;
    };

    std::optional<SandboxTarget> resolveSandboxTarget(
        lua_State* L, int rootIdx, int pathIdx, char const* method, bool requireWritable = false
    );
} // namespace luax
