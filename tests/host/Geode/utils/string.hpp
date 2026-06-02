#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace geode::utils::string {
    inline std::vector<std::string> split(std::string const& value, std::string_view separator) {
        std::vector<std::string> parts;
        std::size_t start = 0;
        while (start <= value.size()) {
            auto pos = value.find(separator, start);
            if (pos == std::string::npos) {
                parts.emplace_back(value.substr(start));
                break;
            }
            parts.emplace_back(value.substr(start, pos - start));
            start = pos + separator.size();
        }
        return parts;
    }

    inline std::string join(std::span<std::string const> parts, std::string_view separator) {
        std::string out;
        for (std::size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) {
                out.append(separator);
            }
            out.append(parts[i]);
        }
        return out;
    }

    inline std::string pathToString(std::filesystem::path const& path) {
        auto text = path.u8string();
        return std::string(reinterpret_cast<char const*>(text.data()), text.size());
    }
}
