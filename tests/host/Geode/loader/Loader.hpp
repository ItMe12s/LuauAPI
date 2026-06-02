#pragma once

#include <Geode/loader/Mod.hpp>

#include <vector>

namespace geode {
    class Loader {
    public:
        static Loader* get() {
            static Loader loader;
            return &loader;
        }

        std::vector<Mod*> getAllMods() const { return Mod::allMods(); }
    };
}
