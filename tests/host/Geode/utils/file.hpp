#pragma once

#include <Geode/Result.hpp>

#include <fstream>
#include <string>

namespace geode::utils::file {
    inline geode::Result<std::string> readString(std::filesystem::path const& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in.good()) {
            return geode::Err<std::string>("read failed");
        }
        return geode::Ok(std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
    }
}
