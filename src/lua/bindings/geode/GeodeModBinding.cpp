#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/TableUtil.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/utils/string.hpp>
#include <matjson.hpp>

#include <lua.h>
#include <lualib.h>

#include <string>
#include <string_view>
#include <vector>

namespace {
    using namespace luax;

    constexpr int kMaxJsonDepth = 32;

    void pushJson(lua_State* L, matjson::Value const& value, int depth);
    matjson::Value toJson(lua_State* L, int idx, int depth);

    void pushString(lua_State* L, std::string const& s) {
        lua_pushlstring(L, s.data(), s.size());
    }

    void pushJson(lua_State* L, matjson::Value const& value, int depth) {
        if (depth > kMaxJsonDepth) {
            lua_pushnil(L);
            return;
        }
        switch (value.type()) {
            case matjson::Type::Bool:
                lua_pushboolean(L, value.asBool().unwrapOr(false));
                break;
            case matjson::Type::Number:
                lua_pushnumber(L, value.asDouble().unwrapOr(0.0));
                break;
            case matjson::Type::String:
                pushString(L, value.asString().unwrapOr(std::string()));
                break;
            case matjson::Type::Array: {
                lua_checkstack(L, 4);
                lua_newtable(L);
                int index = 1;
                for (auto const& item : value) {
                    pushJson(L, item, depth + 1);
                    lua_rawseti(L, -2, index++);
                }
                break;
            }
            case matjson::Type::Object: {
                lua_checkstack(L, 4);
                lua_newtable(L);
                for (auto const& item : value) {
                    auto key = item.getKey();
                    if (!key) continue;
                    pushString(L, *key);
                    pushJson(L, item, depth + 1);
                    lua_rawset(L, -3);
                }
                break;
            }
            case matjson::Type::Null:
            default:
                lua_pushnil(L);
                break;
        }
    }

    bool isLuaArray(lua_State* L, int idx, int& outLen) {
        outLen = lua_objlen(L, idx);
        return outLen > 0;
    }

    matjson::Value toJson(lua_State* L, int idx, int depth) {
        idx = lua_absindex(L, idx);
        switch (lua_type(L, idx)) {
            case LUA_TBOOLEAN:
                return matjson::Value(lua_toboolean(L, idx) != 0);
            case LUA_TNUMBER:
                return matjson::Value(static_cast<double>(lua_tonumber(L, idx)));
            case LUA_TSTRING: {
                std::size_t len = 0;
                char const* s = lua_tolstring(L, idx, &len);
                return matjson::Value(std::string(s, len));
            }
            case LUA_TTABLE: {
                if (depth > kMaxJsonDepth) return matjson::Value(nullptr);
                int len = 0;
                if (isLuaArray(L, idx, len)) {
                    std::vector<matjson::Value> arr;
                    arr.reserve(static_cast<std::size_t>(len));
                    for (int i = 1; i <= len; ++i) {
                        lua_rawgeti(L, idx, i);
                        arr.push_back(toJson(L, -1, depth + 1));
                        lua_pop(L, 1);
                    }
                    return matjson::Value(std::move(arr));
                }
                auto obj = matjson::Value::object();
                lua_pushnil(L);
                while (lua_next(L, idx) != 0) {
                    if (lua_type(L, -2) == LUA_TSTRING) {
                        std::size_t klen = 0;
                        char const* k = lua_tolstring(L, -2, &klen);
                        obj.set(std::string_view(k, klen), toJson(L, -1, depth + 1));
                    }
                    lua_pop(L, 1);
                }
                return obj;
            }
            default:
                return matjson::Value(nullptr);
        }
    }

    std::string_view checkKey(lua_State* L, int idx) {
        std::size_t len = 0;
        char const* key = luaL_checklstring(L, idx, &len);
        return std::string_view(key, len);
    }

    int modGetSavedValue(lua_State* L) {
        auto key = checkKey(L, 1);
        auto result = geode::Mod::get()->getSaveContainer().get(key);
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
        geode::Mod::get()->getSaveContainer().set(key, std::move(value));
        return 0;
    }

    int modGetSettingValue(lua_State* L) {
        auto key = checkKey(L, 1);
        auto result = geode::Mod::get()->getSavedSettingsData().get(key);
        if (result.isErr()) {
            lua_pushnil(L);
            return 1;
        }
        pushJson(L, result.unwrap(), 0);
        return 1;
    }

    int modHasSetting(lua_State* L) {
        lua_pushboolean(L, geode::Mod::get()->hasSetting(checkKey(L, 1)));
        return 1;
    }

    int modGetID(lua_State* L) {
        pushString(L, std::string(geode::Mod::get()->getID()));
        return 1;
    }

    int modGetName(lua_State* L) {
        pushString(L, std::string(geode::Mod::get()->getName()));
        return 1;
    }

    int modGetVersion(lua_State* L) {
        pushString(L, geode::Mod::get()->getVersion().toVString());
        return 1;
    }

    int modGetResourcesDir(lua_State* L) {
        pushString(L, geode::utils::string::pathToString(geode::Mod::get()->getResourcesDir()));
        return 1;
    }

    int modGetSaveDir(lua_State* L) {
        pushString(L, geode::utils::string::pathToString(geode::Mod::get()->getSaveDir()));
        return 1;
    }

    int modGetConfigDir(lua_State* L) {
        pushString(L, geode::utils::string::pathToString(geode::Mod::get()->getConfigDir()));
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
