#pragma once

#include <Geode/loader/Mod.hpp>
#include <string>
#include <vector>

namespace geode {
    class Loader {
    public:
        static Loader* get() {
            static Loader loader;
            return &loader;
        }

        std::vector<Mod*> getAllMods() const {
            return Mod::allMods();
        }

        VersionInfo getVersion() const {
            return VersionInfo("v5.8.1");
        }
    };
} // namespace geode
