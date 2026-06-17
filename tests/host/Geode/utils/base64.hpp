#pragma once

#include <Geode/Result.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace geode::utils::base64 {
    enum class Base64Variant {
        Normal,
        NormalNoPad,
        Url,
        UrlWithPad,
    };

    inline int base64Value(char ch, Base64Variant var) {
        if (ch >= 'A' && ch <= 'Z') {
            return ch - 'A';
        }
        if (ch >= 'a' && ch <= 'z') {
            return ch - 'a' + 26;
        }
        if (ch >= '0' && ch <= '9') {
            return ch - '0' + 52;
        }
        if (ch == '+') {
            return 62;
        }
        if (ch == '/') {
            return 63;
        }
        if (var == Base64Variant::Url || var == Base64Variant::UrlWithPad) {
            if (ch == '-') {
                return 62;
            }
            if (ch == '_') {
                return 63;
            }
        }
        return -1;
    }

    inline geode::Result<std::vector<std::uint8_t>> decode(
        std::string_view str, Base64Variant var = Base64Variant::Url
    ) {
        if (auto const nul = str.find('\0'); nul != std::string_view::npos) {
            str = str.substr(0, nul);
        }
        if (auto const pad = str.find('='); pad != std::string_view::npos) {
            str = str.substr(0, pad);
        }

        std::vector<std::uint8_t> bytes;
        bytes.reserve(str.size() * 3 / 4);

        unsigned int buffer = 0;
        unsigned int bufferBits = 0;

        for (char ch : str) {
            if (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
                continue;
            }

            int const value = base64Value(ch, var);
            if (value < 0) {
                return geode::Err("Invalid base64 character");
            }

            buffer = (buffer << 6) | static_cast<unsigned int>(value);
            bufferBits += 6;
            if (bufferBits >= 8) {
                bufferBits -= 8;
                bytes.push_back(static_cast<std::uint8_t>(buffer >> bufferBits));
            }
        }

        return geode::Ok(std::move(bytes));
    }

    inline geode::Result<std::string> decodeString(
        std::string_view str, Base64Variant var = Base64Variant::Url
    ) {
        auto bytes = decode(str, var);
        if (bytes.isErr()) {
            return geode::Err(bytes.unwrapErr());
        }
        auto const& decoded = bytes.unwrap();
        return geode::Ok(std::string(
            reinterpret_cast<char const*>(decoded.data()),
            reinterpret_cast<char const*>(decoded.data() + decoded.size())
        ));
    }

    inline std::string encode(std::span<std::uint8_t const> /*data*/, Base64Variant /*var*/) {
        return {};
    }

    inline std::string encode(std::string_view /*str*/, Base64Variant /*var*/) {
        return {};
    }
}
