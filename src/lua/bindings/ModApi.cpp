#include "../Binding.hpp"
#include "../Runtime.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"

#include <lua.h>

namespace {
    using namespace luax;

    int modResourcesPath(lua_State* L) {
        push(L, luax::Runtime::instance().resourcesRoot().string());
        return 1;
    }

    void bindModApi(lua_State* L) {
        getOrCreateTable(L, "geode");
        lua_pushcfunction(L, &modResourcesPath, "modResourcesPath");
        lua_setfield(L, -2, "modResourcesPath");
        lua_pop(L, 1);
    }

    LUAX_BINDING(ModApi, bindModApi)
}
