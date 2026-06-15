#include "bindings/geode/CurrentMod.hpp"

#include "core/Runtime.hpp"
#include "require/PathSandbox.hpp"
#include "require/VirtualChunk.hpp"

#include <Geode/loader/Mod.hpp>
#include <lua.h>
#include <string>
#include <unordered_map>

namespace {
    std::unordered_map<std::string, geode::Mod*>& modCache() {
        static std::unordered_map<std::string, geode::Mod*> cache;
        return cache;
    }
} // namespace

namespace luax {
    void invalidateCurrentModCache() {
        modCache().clear();
    }

    geode::Mod* currentMod() {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) {
            return nullptr;
        }

        auto const& root = runtime->resourcesRoot();
        if (root.empty()) {
            return nullptr;
        }

        auto rootResult = canonicalRoot(root);
        if (rootResult.isErr()) {
            return nullptr;
        }
        // currentMod comes from active script resources only.
        auto const& canonical = rootResult.unwrap();
        auto key = filesystemPathString(canonical);

        auto& cache = modCache();
        if (auto it = cache.find(key); it != cache.end()) {
            if (modForResourcesRoot(canonical) == it->second) {
                return it->second;
            }
            cache.erase(it);
        }

        geode::Mod* found = modForResourcesRoot(canonical);
        if (found) {
            cache[key] = found;
        }
        return found;
    }

    geode::Mod* requireCurrentMod(lua_State* L) {
        auto* mod = currentMod();
        if (!mod) {
            luaL_error(L, "current mod is unavailable");
        }
        return mod;
    }
} // namespace luax
