#include "CurrentMod.hpp"

#include "lua/module/PathSandbox.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>

#include <string>
#include <unordered_map>

namespace luax {
    geode::Mod* currentMod() {
        auto* runtime = luax::Runtime::getIfInitialized();
        if (!runtime) {
            return geode::Mod::get();
        }
        auto const& root = runtime->resourcesRoot();
        if (root.empty()) {
            return geode::Mod::get();
        }

        static std::unordered_map<std::string, geode::Mod*> cache;
        auto key = normalizedPathString(root);
        if (auto it = cache.find(key); it != cache.end()) {
            return it->second ? it->second : geode::Mod::get();
        }

        geode::Mod* found = nullptr;
        for (auto* mod : geode::Loader::get()->getAllMods()) {
            auto modRoot = canonicalRoot(mod->getResourcesDir());
            if (modRoot.isOk() && modRoot.unwrap() == root) {
                found = mod;
                break;
            }
        }
        cache[key] = found;
        return found ? found : geode::Mod::get();
    }
}
