#include "core/StackFormat.hpp"

#include <Geode/utils/general.hpp>
#include <Geode/utils/string.hpp>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

namespace luax {
    namespace {
        std::size_t findRootPrefix(std::string const& text, std::string_view rootText, std::size_t pos) {
            if (rootText.empty() || pos >= text.size()) return std::string::npos;
#if defined(_WIN32)
            for (std::size_t scan = pos; scan + rootText.size() <= text.size(); ++scan) {
                if (geode::utils::string::equalsIgnoreCase(
                        std::string_view(text).substr(scan, rootText.size()), rootText
                    )) {
                    return scan;
                }
            }
            return std::string::npos;
#else
            return text.find(rootText, pos);
#endif
        }

        void replaceRootPrefix(std::string& text, std::string_view rootText) {
            if (rootText.empty()) return;
            std::size_t pos = 0;
            while (pos < text.size()) {
                std::size_t const found = findRootPrefix(text, rootText, pos);
                if (found == std::string::npos) break;

                std::size_t tail = found + rootText.size();
                while (tail < text.size() && (text[tail] == '/' || text[tail] == '\\')) {
                    ++tail;
                }
                text.replace(found, tail - found, "@");
                pos = found + 1;
            }
        }
    } // namespace

    std::string formatDebugSource(char const* source, std::filesystem::path const& resourcesRoot) {
        if (!source || !source[0]) return {};

        std::string_view text(source);
        if (!text.empty() && (text.front() == '@' || text.front() == '=')) {
            text.remove_prefix(1);
        }

        std::filesystem::path filePath(text);
        if (filePath.empty()) return source;

        if (!resourcesRoot.empty() && filePath.is_absolute()) {
            auto rootResult = canonicalRoot(resourcesRoot);
            if (rootResult.isOk()) {
                std::error_code ec;
                auto resolved = std::filesystem::weakly_canonical(filePath, ec);
                if (!ec && pathInsideRootValue(resolved, rootResult.unwrap())) {
                    auto rel = resolved.lexically_relative(rootResult.unwrap());
                    return "@" + normalizedPathString(rel);
                }
            }
        }

        if (filePath.is_absolute()) {
            auto name = filePath.filename();
            if (!name.empty()) {
                return "@" + normalizedPathString(name);
            }
        }

        return source;
    }

    std::string redactHostPaths(std::string_view text, std::filesystem::path const& resourcesRoot) {
        std::string out(text);
        if (resourcesRoot.empty()) return out;

        auto rootResult = canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) return out;

        auto rootText = filesystemPathString(rootResult.unwrap());
        replaceRootPrefix(out, rootText);

        auto genericRoot = normalizedPathString(rootResult.unwrap());
        if (genericRoot != rootText) {
            replaceRootPrefix(out, genericRoot);
        }

        return out;
    }

    namespace {
        void appendStackFrame(
            std::string& out, lua_Debug const& ar, std::filesystem::path const& resourcesRoot
        ) {
            out.append("\n    ");
            if (ar.source) {
                out.append(formatDebugSource(ar.short_src, resourcesRoot));
            }
            if (ar.currentline > 0) {
                out.append(":");
                out.append(geode::utils::numToString(ar.currentline));
            }
            if (ar.name) {
                out.append(" in ");
                out.append(ar.name);
            }
        }
    } // namespace

    std::string formatPendingCall(lua_State* L, int funcIndex, std::filesystem::path const& resourcesRoot) {
        if (!L || funcIndex == 0) return {};

        lua_Debug ar;
        if (!lua_getinfo(L, funcIndex, "sln", &ar)) return {};

        std::string out;
        out.append("  stack:");
        appendStackFrame(out, ar, resourcesRoot);
        return out;
    }

    bool hasStackFrames(std::string_view luaStack) {
        return geode::utils::string::contains(luaStack, '\n');
    }

    std::string formatLuaStack(lua_State* L, std::filesystem::path const& resourcesRoot) {
        if (!L) return {};

        std::string out;
        out.append("  stack:");
        lua_Debug ar;
        for (int level = 0; lua_getinfo(L, level, "sln", &ar); ++level) {
            appendStackFrame(out, ar, resourcesRoot);
        }
        return hasStackFrames(out) ? out : std::string{};
    }
} // namespace luax
