#include "require/BytecodeCacheKey.hpp"

#include "require/PathSandbox.hpp"

#if !defined(LUAUAPI_HOST_TESTS)
    #include <Geode/utils/general.hpp>
    #include <Geode/utils/string.hpp>
#endif

#include <chrono>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace luax {
    namespace {
#if defined(LUAUAPI_HOST_TESTS)
        std::string contentHashToken(std::string_view source) {
            return std::to_string(std::hash<std::string_view>{}(source));
        }

        template <class Num>
        std::string numericToken(Num value) {
            return std::to_string(value);
        }

        std::string joinKeyParts(std::vector<std::string> const& parts, std::string_view separator) {
            std::string out;
            for (std::size_t i = 0; i < parts.size(); ++i) {
                if (i > 0) {
                    out.append(separator);
                }
                out.append(parts[i]);
            }
            return out;
        }
#else
        std::string contentHashToken(std::string_view source) {
            return geode::utils::numToString(geode::utils::hash(source));
        }

        template <class Num>
        std::string numericToken(Num value) {
            return geode::utils::numToString(value);
        }

        std::string joinKeyParts(std::vector<std::string> const& parts, std::string_view separator) {
            return geode::utils::string::join(std::span{parts.data(), parts.size()}, separator);
        }
#endif

    } // namespace

    std::string fileCacheKey(std::filesystem::path const& path, std::string const& contents) {
        std::error_code ec;
        auto size = std::filesystem::file_size(path, ec);
        if (ec) {
            size = 0;
        }

        auto stamp = std::filesystem::last_write_time(path, ec);
        std::int64_t stampNanoseconds = 0;
        if (!ec) {
            stampNanoseconds = static_cast<std::int64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(stamp.time_since_epoch()).count()
            );
        }

        return joinKeyParts(
            {
                filesystemPathString(path),
                "size=" + numericToken(size),
                "mtime=" + numericToken(stampNanoseconds),
                "hash=" + contentHashToken(contents),
            },
            "|"
        );
    }

    std::string loadstringBytecodeKey(std::string_view chunkName, std::string_view source) {
        return joinKeyParts(
            {
                std::string(chunkName),
                "size=" + numericToken(source.size()),
                "hash=" + contentHashToken(source),
            },
            "|"
        );
    }

    std::string runScriptBytecodeKey(
        std::filesystem::path const& resourcesRoot, std::string_view chunkName, std::string_view source
    ) {
        std::vector<std::string> parts;
        if (!resourcesRoot.empty()) {
            parts.push_back(filesystemPathString(resourcesRoot));
        }
        parts.push_back(std::string(chunkName));
        parts.push_back(contentHashToken(source));
        return joinKeyParts(parts, "|");
    }
} // namespace luax
