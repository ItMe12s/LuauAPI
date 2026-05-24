#pragma once

#include "PathSandbox.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <system_error>

namespace luax {
    inline std::string bytecodeCacheKey(std::filesystem::path const& path, std::string const& contents) {
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (ec) size = 0;

        auto stamp = std::filesystem::last_write_time(path, ec);
        auto stampTicks = ec ? 0 : stamp.time_since_epoch().count();

        return normalizedPathString(path)
            + "|size=" + std::to_string(size)
            + "|mtime=" + std::to_string(stampTicks)
            + "|hash=" + std::to_string(std::hash<std::string>{}(contents));
    }
}
