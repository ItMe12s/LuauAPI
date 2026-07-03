#pragma once

#include <chrono>
#include <filesystem>
#include <string>

namespace geode::dirs {
    inline std::filesystem::path& crashlogsDirStorage() {
        static std::filesystem::path dir = [] {
            auto p = std::filesystem::temp_directory_path() /
                ("luauapi_test_crashlogs_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            std::filesystem::create_directories(p);
            return p;
        }();
        return dir;
    }

    inline std::filesystem::path getCrashlogsDir() {
        return crashlogsDirStorage();
    }
} // namespace geode::dirs
