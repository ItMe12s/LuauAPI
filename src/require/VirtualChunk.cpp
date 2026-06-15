#include "require/VirtualChunk.hpp"

#include "require/PathSandbox.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/string.hpp>

namespace luax {
    namespace {
        bool modStillRegistered(geode::Mod* mod) {
            if (!mod) {
                return false;
            }
            for (auto* candidate : geode::Loader::get()->getAllMods()) {
                if (candidate == mod) {
                    return true;
                }
            }
            return false;
        }

        bool modMatchesRoot(geode::Mod* mod, std::filesystem::path const& root) {
            if (!modStillRegistered(mod)) {
                return false;
            }
            auto modRoot = canonicalRoot(mod->getResourcesDir());
            return modRoot.isOk() && modRoot.unwrap() == root;
        }

        geode::Mod* findModForRoot(std::filesystem::path const& root) {
            for (auto* mod : geode::Loader::get()->getAllMods()) {
                if (modMatchesRoot(mod, root)) {
                    return mod;
                }
            }
            return nullptr;
        }
    } // namespace

    std::string_view stripVirtualChunkAt(std::string_view chunk) {
        if (geode::utils::string::startsWith(chunk, "@")) {
            chunk.remove_prefix(1);
        }
        return chunk;
    }

    VirtualChunkParts parseVirtualChunk(std::string_view chunk) {
        chunk = stripVirtualChunkAt(chunk);
        auto separator = chunk.find(kVirtualChunkModSeparator);
        if (separator == std::string_view::npos || separator == 0) {
            return VirtualChunkParts{{}, chunk};
        }

        return VirtualChunkParts{chunk.substr(0, separator), chunk.substr(separator + 1)};
    }

    std::string_view virtualChunkScriptPath(std::string_view chunk) {
        return parseVirtualChunk(chunk).scriptPath;
    }

    bool virtualChunkHasModPrefix(std::string_view chunk) {
        auto parts = parseVirtualChunk(chunk);
        return !parts.modId.empty();
    }

    std::string formatVirtualChunk(std::string_view modId, std::string_view scriptPath) {
        scriptPath = stripVirtualChunkAt(scriptPath);
        if (auto nested = parseVirtualChunk(scriptPath); !nested.modId.empty()) {
            scriptPath = nested.scriptPath;
        }

        std::string out = "@";
        out.append(modId);
        out.push_back(kVirtualChunkModSeparator);
        out.append(scriptPath);
        return out;
    }

    geode::Mod* modForResourcesRoot(std::filesystem::path const& resourcesRoot) {
        if (resourcesRoot.empty()) {
            return nullptr;
        }
        auto rootResult = canonicalRoot(resourcesRoot);
        if (rootResult.isErr()) {
            return nullptr;
        }
        return findModForRoot(rootResult.unwrap());
    }

    std::optional<std::string> modIdForResourcesRoot(std::filesystem::path const& resourcesRoot) {
        auto* mod = modForResourcesRoot(resourcesRoot);
        if (!mod) {
            return std::nullopt;
        }
        return std::string(mod->getID());
    }

    std::string enrichCallbackContext(
        std::filesystem::path const& resourcesRoot, std::string_view context
    ) {
        if (!context.empty() && context.front() == '@') {
            return std::string(context);
        }
        if (auto modId = modIdForResourcesRoot(resourcesRoot)) {
            std::string out;
            out.reserve(modId->size() + 1 + context.size());
            out.append(*modId);
            out.push_back(kVirtualChunkModSeparator);
            out.append(context);
            return out;
        }
        return std::string(context);
    }
} // namespace luax
