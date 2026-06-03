#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/CurrentMod.hpp"
#include "lua/bindings/geode/JsonConvert.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/utils/string.hpp>
#include <matjson.hpp>

#include <lua.h>
#include <lualib.h>

#include <string>
#include <string_view>

namespace {
    using namespace luax;

    std::string_view checkKey(lua_State* L, int idx) {
        std::size_t len = 0;
        char const* key = luaL_checklstring(L, idx, &len);
        return std::string_view(key, len);
    }

    int modGetSavedValue(lua_State* L) {
        auto key = checkKey(L, 1);
        auto result = requireCurrentMod(L)->getSaveContainer().get(key);
        if (result.isErr()) {
            lua_pushnil(L);
            return 1;
        }
        pushJson(L, result.unwrap(), 0);
        return 1;
    }

    int modSetSavedValue(lua_State* L) {
        auto key = checkKey(L, 1);
        auto value = toJson(L, 2, 0);
        requireCurrentMod(L)->getSaveContainer().set(key, std::move(value));
        return 0;
    }

    int modGetSettingValue(lua_State* L) {
        auto key = checkKey(L, 1);
        auto result = requireCurrentMod(L)->getSavedSettingsData().get(key);
        if (result.isErr()) {
            lua_pushnil(L);
            return 1;
        }
        pushJson(L, result.unwrap(), 0);
        return 1;
    }

    int modHasSetting(lua_State* L) {
        lua_pushboolean(L, requireCurrentMod(L)->hasSetting(checkKey(L, 1)));
        return 1;
    }

    int modGetID(lua_State* L) {
        push(L, std::string(requireCurrentMod(L)->getID()));
        return 1;
    }

    int modGetName(lua_State* L) {
        push(L, std::string(requireCurrentMod(L)->getName()));
        return 1;
    }

    int modGetVersion(lua_State* L) {
        push(L, requireCurrentMod(L)->getVersion().toVString());
        return 1;
    }

    int modGetResourcesDir(lua_State* L) {
        push(L, geode::utils::string::pathToString(requireCurrentMod(L)->getResourcesDir()));
        return 1;
    }

    int modGetSaveDir(lua_State* L) {
        push(L, geode::utils::string::pathToString(requireCurrentMod(L)->getSaveDir()));
        return 1;
    }

    int modGetConfigDir(lua_State* L) {
        push(L, geode::utils::string::pathToString(requireCurrentMod(L)->getConfigDir()));
        return 1;
    }

    geode::Result<void> registerGeodeMod(lua_State* L) {
        luax::getOrCreateTable(L, "geode");
        lua_newtable(L);
        setTableCFunction(L, -1, "getSavedValue", &modGetSavedValue);
        setTableCFunction(L, -1, "setSavedValue", &modSetSavedValue);
        setTableCFunction(L, -1, "getSettingValue", &modGetSettingValue);
        setTableCFunction(L, -1, "hasSetting", &modHasSetting);
        setTableCFunction(L, -1, "getID", &modGetID);
        setTableCFunction(L, -1, "getName", &modGetName);
        setTableCFunction(L, -1, "getVersion", &modGetVersion);
        setTableCFunction(L, -1, "getResourcesDir", &modGetResourcesDir);
        setTableCFunction(L, -1, "getSaveDir", &modGetSaveDir);
        setTableCFunction(L, -1, "getConfigDir", &modGetConfigDir);
        lua_setfield(L, -2, "Mod");
        lua_pop(L, 1);
        return geode::Ok();
    }
}

LUAX_BINDING(geode_mod_lib, registerGeodeMod)
