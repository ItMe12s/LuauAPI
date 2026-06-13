#pragma once

#include <Geode/Result.hpp>
#include <filesystem>
#include <optional>
#include <string>

struct lua_State;

namespace luax {
    struct SandboxTarget {
        std::filesystem::path path;
        bool writable;
    };

    std::optional<SandboxTarget> resolveSandboxTarget(
        lua_State* L, int rootIdx, int pathIdx, char const* method, bool requireWritable = false
    );

    geode::Result<std::string> readSandboxTextFile(std::filesystem::path const& path);
} // namespace luax
