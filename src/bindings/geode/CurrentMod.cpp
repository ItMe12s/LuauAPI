#include "bindings/geode/CurrentMod.hpp"

#include "core/Runtime.hpp"
#include "require/PathSandbox.hpp"

#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <lua.h>
#include <string>
#include <unordered_map>

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
        auto modRoot = luax::canonicalRoot(mod->getResourcesDir());
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
            if (modMatchesRoot(it->second, canonical)) {
                return it->second;
            }
            cache.erase(it);
        }

        geode::Mod* found = findModForRoot(canonical);
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
