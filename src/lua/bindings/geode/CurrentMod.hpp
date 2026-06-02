#pragma once

struct lua_State;

namespace geode {
    class Mod;
}

namespace luax {
    geode::Mod* currentMod();
    void invalidateCurrentModCache();
    geode::Mod* requireCurrentMod(lua_State* L);
}
