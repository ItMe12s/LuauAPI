#include "lua/Config.hpp"
#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/LuaCallback.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/CurrentMod.hpp"
#include "lua/bindings/geode/JsonConvert.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/utils/string.hpp>
#include <lua.h>
#include <lualib.h>
#include <matjson.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {
    using namespace luax;

    std::string_view checkKey(lua_State* L, int idx) {
        std::size_t len = 0;
        char const* key = luaL_checklstring(L, idx, &len);
        return std::string_view(key, len);
    }

    std::vector<geode::ListenerHandle>& settingListeners() {
        static auto* s = new std::vector<geode::ListenerHandle>();
        return *s;
    }

    bool s_settingShutdownHookRegistered = false;

    void clearSettingListeners() {
        settingListeners().clear();
        s_settingShutdownHookRegistered = false;
    }

    void ensureSettingShutdownHook() {
        if (s_settingShutdownHookRegistered) return;
        auto* rt = Runtime::getIfInitialized();
        if (!rt) return;
        rt->registerShutdownHook(&clearSettingListeners);
        s_settingShutdownHookRegistered = true;
    }

    void pushSettingValue(lua_State* L, std::shared_ptr<geode::SettingV3> const& setting) {
        matjson::Value value;
        if (!setting || !setting->save(value)) {
            lua_pushnil(L);
            return;
        }
        if (auto pushed = pushJson(L, value, 0); pushed.isErr()) {
            geode::log::warn(
                "geode.Mod setting value exceeds json depth limit: {}", pushed.unwrapErr()
            );
            lua_pushnil(L);
        }
    }

    int modGetSavedValue(lua_State* L) {
        auto key = checkKey(L, 1);
        auto result = requireCurrentMod(L)->getSaveContainer().get(key);
        if (result.isErr()) {
            lua_pushnil(L);
            return 1;
        }
        if (auto pushed = pushJson(L, result.unwrap(), 0); pushed.isErr()) {
            lua_pushnil(L);
            push(L, std::string(pushed.unwrapErr()));
            return 2;
        }
        return 1;
    }

    int modSetSavedValue(lua_State* L) {
        auto key = checkKey(L, 1);
        auto valueResult = toJson(L, 2, 0);
        if (valueResult.isErr()) {
            lua_pushnil(L);
            push(L, std::string(valueResult.unwrapErr()));
            return 2;
        }
        auto& saved = requireCurrentMod(L)->getSaveContainer();
        if (!saved.isObject()) {
            lua_pushnil(L);
            push(L, std::string("save container is not an object"));
            return 2;
        }
        saved.set(key, std::move(valueResult.unwrap()));
        push(L, true);
        return 1;
    }

    int modGetSettingValue(lua_State* L) {
        auto key = checkKey(L, 1);
        auto result = requireCurrentMod(L)->getSavedSettingsData().get(key);
        if (result.isErr()) {
            lua_pushnil(L);
            return 1;
        }
        if (auto pushed = pushJson(L, result.unwrap(), 0); pushed.isErr()) {
            lua_pushnil(L);
            push(L, std::string(pushed.unwrapErr()));
            return 2;
        }
        return 1;
    }

    int modHasSetting(lua_State* L) {
        lua_pushboolean(L, requireCurrentMod(L)->hasSetting(checkKey(L, 1)));
        return 1;
    }

    int modListenForSettingChanges(lua_State* L) {
        auto key = std::string(checkKey(L, 1));
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto* mod = requireCurrentMod(L);
        auto cb = std::make_shared<luax::LuaCallback>(L, 2);

        settingListeners().push_back(
            geode::SettingChangedEventV3(mod, key).listen(
                [cb](std::shared_ptr<geode::SettingV3> setting) {
                    if (!cb->valid()) return;
                    struct Ctx {
                        std::shared_ptr<geode::SettingV3> const* s;
                    } ctx{&setting};
                    if (!cb->invoke(
                            1,
                            0,
                            "geode.Mod.listenForSettingChanges",
                            kHookScriptDeadlineMs,
                            +[](lua_State* L, void* raw) {
                                pushSettingValue(L, *static_cast<Ctx*>(raw)->s);
                            },
                            &ctx
                        )) {
                        logCallbackFailure("geode.Mod.listenForSettingChanges");
                    }
                }
            )
        );
        ensureSettingShutdownHook();
        return 0;
    }

    int modListenForAllSettingChanges(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto modID = std::string(requireCurrentMod(L)->getID());
        auto cb = std::make_shared<luax::LuaCallback>(L, 1);

        settingListeners().push_back(
            geode::SettingChangedEventV3().listen(
                [cb, modID](
                    std::string_view evModID, std::string_view key, std::shared_ptr<geode::SettingV3> setting
                ) {
                    if (evModID != modID || !cb->valid()) return;
                    struct Ctx {
                        std::string_view key;
                        std::shared_ptr<geode::SettingV3> const* s;
                    } ctx{key, &setting};
                    if (!cb->invoke(
                            2,
                            0,
                            "geode.Mod.listenForAllSettingChanges",
                            kHookScriptDeadlineMs,
                            +[](lua_State* L, void* raw) {
                                auto* c = static_cast<Ctx*>(raw);
                                lua_pushlstring(L, c->key.data(), c->key.size());
                                pushSettingValue(L, *c->s);
                            },
                            &ctx
                        )) {
                        logCallbackFailure("geode.Mod.listenForAllSettingChanges");
                    }
                }
            )
        );
        ensureSettingShutdownHook();
        return 0;
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
        setTableCFunction(L, -1, "listenForSettingChanges", &modListenForSettingChanges);
        setTableCFunction(L, -1, "listenForAllSettingChanges", &modListenForAllSettingChanges);
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
} // namespace

LUAX_BINDING(geode_mod_lib, registerGeodeMod)
