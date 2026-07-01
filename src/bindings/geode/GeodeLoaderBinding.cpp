#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/string.hpp>
#include <lua.h>
#include <span>
#include <string>

namespace {
    using namespace luax;

    int loaderGetAllMods(lua_State* L) {
        auto mods = geode::Loader::get()->getAllMods();
        lua_createtable(L, static_cast<int>(mods.size()), 0);
        int index = 1;
        for (auto* mod : mods) {
            if (!mod) continue;
            lua_createtable(L, 0, 6);
            push(L, std::string(mod->getID()));
            lua_setfield(L, -2, "id");
            push(L, std::string(mod->getName()));
            lua_setfield(L, -2, "name");
            auto const& devs = mod->getDevelopers();
            push(L, geode::utils::string::join(std::span{devs.data(), devs.size()}, ", "));
            lua_setfield(L, -2, "developer");
            push(L, mod->getVersion().toVString());
            lua_setfield(L, -2, "version");
            push(L, mod->isLoaded());
            lua_setfield(L, -2, "enabled");
            push(L, mod->shouldLoad());
            lua_setfield(L, -2, "shouldLoad");
            lua_rawseti(L, -2, index++);
        }
        return 1;
    }

    geode::Result<void> registerGeodeLoader(lua_State* L) {
        luax::getOrCreateTable(L, "geode");
        lua_newtable(L);
        setTableCFunction(L, -1, "getAllMods", &loaderGetAllMods);
        lua_setfield(L, -2, "Loader");
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace

LUAX_BINDING(geode_loader_lib, registerGeodeLoader)
