#pragma once

#include <Geode/Result.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace geode::utils::file {
    inline geode::Result<std::string> readString(std::filesystem::path const& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in.good()) {
            return geode::Err("read failed");
        }
        return geode::Ok(
            std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>())
        );
    }

    inline geode::Result<std::vector<std::uint8_t>> readBinary(std::filesystem::path const& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in.good()) {
            return geode::Err("read failed");
        }
        return geode::Ok(
            std::vector<std::uint8_t>(
                (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()
            )
        );
    }

    inline geode::Result<void> writeString(std::filesystem::path const& path, std::string const& data) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out.good()) {
            return geode::Err("write failed");
        }
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (!out.good()) {
            return geode::Err("write failed");
        }
        return geode::Ok();
    }

    inline geode::Result<void> createDirectoryAll(std::filesystem::path const& path) {
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
        if (ec) {
            return geode::Err(ec.message());
        }
        return geode::Ok();
    }

    inline geode::Result<std::vector<std::filesystem::path>> readDirectory(
        std::filesystem::path const& path, bool /*recursive*/
    ) {
        std::vector<std::filesystem::path> entries;
        std::error_code ec;
        for (auto const& entry : std::filesystem::directory_iterator(path, ec)) {
            if (ec) {
                return geode::Err(ec.message());
            }
            entries.push_back(entry.path());
        }
        return geode::Ok(std::move(entries));
    }
} // namespace geode::utils::file
