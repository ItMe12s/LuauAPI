#pragma once

#include <Geode/Result.hpp>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

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
    geode::Result<std::vector<std::uint8_t>> readSandboxBinaryFile(std::filesystem::path const& path);
} // namespace luax
