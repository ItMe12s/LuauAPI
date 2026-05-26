#pragma once

#include "PathSandbox.hpp"

#include <Geode/utils/general.hpp>

#include <chrono>
#include <cstdint>
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
        std::int64_t stampNanoseconds = 0;
        if (!ec) {
            stampNanoseconds = static_cast<std::int64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(stamp.time_since_epoch()).count());
        }

        return filesystemPathString(path)
            + "|size=" + geode::utils::numToString(size)
            + "|mtime=" + geode::utils::numToString(stampNanoseconds)
            + "|hash=" + geode::utils::numToString(std::hash<std::string>{}(contents));
    }
}
