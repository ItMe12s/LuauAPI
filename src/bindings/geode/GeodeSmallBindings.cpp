#include "bindings/geode/JsonConvert.hpp"
#include "core/Config.hpp"
#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <lua.h>
#include <lualib.h>
#include <matjson.hpp>
#include <string>
#include <string_view>

#ifndef LUAUAPI_HOST_TESTS
    #include "framework/callback/LuaCallback.hpp"
    #include "framework/stack/Types.hpp"
    #include "framework/usertype/Usertype.hpp"

    #include <Geode/Geode.hpp>
    #include <Geode/utils/ColorProvider.hpp>
    #include <Geode/utils/Keyboard.hpp>
    #include <Geode/utils/VersionInfo.hpp>
    #include <Geode/utils/base64.hpp>
    #include <Geode/utils/permission.hpp>
    #include <cocos2d.h>
    #include <cstdint>
    #include <memory>
#endif

namespace {
    using namespace luax;

#ifndef LUAUAPI_HOST_TESTS
    // geode.utils.base64

    namespace base64 = geode::utils::base64;

    base64::Base64Variant optVariant(lua_State* L, int idx, base64::Base64Variant def) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return def;
        int value = check<int>(L, idx, "geode.utils.base64");
        if (value < 0 || value > static_cast<int>(base64::Base64Variant::UrlWithPad)) {
            luaL_error(
                L, "geode.utils.base64: invalid variant %d (use geode.utils.base64.Variant.*)", value
            );
        }
        return static_cast<base64::Base64Variant>(value);
    }

    int b64Encode(lua_State* L) {
        auto data = check<std::string>(L, 1, "geode.utils.base64.encode");
        auto var = optVariant(L, 2, base64::Base64Variant::UrlWithPad);
        push(L, base64::encode(std::string_view(data), var));
        return 1;
    }

    int b64Decode(lua_State* L) {
        auto data = check<std::string>(L, 1, "geode.utils.base64.decode");
        auto var = optVariant(L, 2, base64::Base64Variant::Url);
        auto result = base64::decode(std::string_view(data), var);
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }
        auto const& bytes = result.unwrap();
        lua_pushlstring(L, reinterpret_cast<char const*>(bytes.data()), bytes.size());
        return 1;
    }

    int b64DecodeString(lua_State* L) {
        auto data = check<std::string>(L, 1, "geode.utils.base64.decodeString");
        auto var = optVariant(L, 2, base64::Base64Variant::Url);
        auto result = base64::decodeString(std::string_view(data), var);
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }
        push(L, result.unwrap());
        return 1;
    }
#endif

    // geode.json

    int jsonParse(lua_State* L) {
        size_t textLen = 0;
        char const* textData = luaL_checklstring(L, 1, &textLen);
        if (textLen > kMaxJsonParseBytes) {
            return pushNilErr(L, "json exceeds maximum size");
        }
        auto result = matjson::Value::parse(std::string_view(textData, textLen));
        if (result.isErr()) {
            return pushNilErr(L, std::string(result.unwrapErr()));
        }
        if (auto pushed = pushJson(L, result.unwrap(), 0); pushed.isErr()) {
            return pushNilErr(L, pushed.unwrapErr());
        }
        return 1;
    }

    int jsonDump(lua_State* L) {
        int indent = matjson::NO_INDENTATION;
        if (!lua_isnoneornil(L, 2)) {
            indent = static_cast<int>(luaL_checkinteger(L, 2));
        }
        auto valueResult = toJson(L, 1, 0);
        if (valueResult.isErr()) {
            luaL_error(L, "%s", valueResult.unwrapErr().c_str());
        }
        push(L, valueResult.unwrap().dump(indent));
        return 1;
    }

#ifndef LUAUAPI_HOST_TESTS
    // geode.VersionInfo

    int viParse(lua_State* L) {
        auto str = check<std::string>(L, 1, "geode.VersionInfo.parse");
        auto result = geode::VersionInfo::parse(str);
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }
        auto version = result.unwrap();
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, static_cast<int>(version.getMajor()));
        lua_setfield(L, -2, "major");
        lua_pushinteger(L, static_cast<int>(version.getMinor()));
        lua_setfield(L, -2, "minor");
        lua_pushinteger(L, static_cast<int>(version.getPatch()));
        lua_setfield(L, -2, "patch");
        return 1;
    }

    int viCompare(lua_State* L) {
        auto a = check<std::string>(L, 1, "geode.VersionInfo.compare");
        auto b = check<std::string>(L, 2, "geode.VersionInfo.compare");
        auto ra = geode::VersionInfo::parse(a);
        if (ra.isErr()) {
            return pushNilErr(L, ra.unwrapErr());
        }
        auto rb = geode::VersionInfo::parse(b);
        if (rb.isErr()) {
            return pushNilErr(L, rb.unwrapErr());
        }
        auto va = ra.unwrap();
        auto vb = rb.unwrap();
        int order = va < vb ? -1 : (va == vb ? 0 : 1);
        push(L, order);
        return 1;
    }

    int viMatches(lua_State* L) {
        auto constraint = check<std::string>(L, 1, "geode.VersionInfo.matches");
        auto version = check<std::string>(L, 2, "geode.VersionInfo.matches");
        auto rc = geode::ComparableVersionInfo::parse(constraint);
        if (rc.isErr()) {
            return pushNilErr(L, rc.unwrapErr());
        }
        auto rv = geode::VersionInfo::parse(version);
        if (rv.isErr()) {
            return pushNilErr(L, rv.unwrapErr());
        }
        push(L, rc.unwrap().compare(rv.unwrap()));
        return 1;
    }

    // geode.utils.permission

    namespace permission = geode::utils::permission;

    permission::Permission checkPermission(lua_State* L, int idx, char const* method) {
        int value = check<int>(L, idx, method);
        if (value != static_cast<int>(permission::Permission::ReadAllFiles) &&
            value != static_cast<int>(permission::Permission::RecordAudio)) {
            luaL_error(
                L, "%s: invalid permission %d (use geode.utils.permission.Permission.*)", method, value
            );
        }
        return static_cast<permission::Permission>(value);
    }

    int permGetStatus(lua_State* L) {
        auto perm = checkPermission(L, 1, "geode.utils.permission.getPermissionStatus");
        push(L, permission::getPermissionStatus(perm));
        return 1;
    }

    int permRequest(lua_State* L) {
        auto perm = checkPermission(L, 1, "geode.utils.permission.requestPermission");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<luax::LuaCallback>(L, 2);
        permission::requestPermission(perm, [cb](bool granted) {
            geode::queueInMainThread([cb, granted] {
                if (!cb || !cb->valid()) return;
                bool g = granted;
                if (!cb->invoke(
                        1,
                        0,
                        "geode.utils.permission.requestPermission",
                        kHookScriptDeadlineMs,
                        +[](lua_State* L, void* raw) {
                            lua_pushboolean(L, *static_cast<bool*>(raw));
                        },
                        &g
                    )) {
                    logCallbackFailure("geode.utils.permission.requestPermission");
                }
            });
        });
        return 0;
    }

    // geode.ColorProvider

    int cpDefine(lua_State* L) {
        auto id = check<std::string>(L, 1, "geode.ColorProvider.define");
        auto color = check<cocos2d::ccColor4B>(L, 2, "geode.ColorProvider.define");
        push(L, geode::ColorProvider::get()->define(id, color));
        return 1;
    }

    int cpOverride(lua_State* L) {
        auto id = check<std::string>(L, 1, "geode.ColorProvider.override");
        auto color = check<cocos2d::ccColor4B>(L, 2, "geode.ColorProvider.override");
        push(L, geode::ColorProvider::get()->override(id, color));
        return 1;
    }

    int cpReset(lua_State* L) {
        auto id = check<std::string>(L, 1, "geode.ColorProvider.reset");
        push(L, geode::ColorProvider::get()->reset(id));
        return 1;
    }

    int cpColor(lua_State* L) {
        auto id = check<std::string>(L, 1, "geode.ColorProvider.color");
        push(L, geode::ColorProvider::get()->color(id));
        return 1;
    }

    int cpColor3b(lua_State* L) {
        auto id = check<std::string>(L, 1, "geode.ColorProvider.color3b");
        push(L, geode::ColorProvider::get()->color3b(id));
        return 1;
    }

    // geode.Keybind

    int intField(lua_State* L, int tableIdx, char const* key, int def, bool required, char const* method) {
        lua_getfield(L, tableIdx, key);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            if (required) luaL_error(L, "%s expected number field '%s'", method, key);
            return def;
        }
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            luaL_error(L, "%s expected number field '%s'", method, key);
        }
        int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    geode::Keybind buildKeybind(lua_State* L, int idx, char const* method) {
        luaL_checktype(L, idx, LUA_TTABLE);
        int key = intField(L, idx, "key", 0, true, method);
        int modifiers = intField(L, idx, "modifiers", 0, false, method);
        return geode::Keybind(
            static_cast<cocos2d::enumKeyCodes>(key),
            geode::KeyboardModifier(static_cast<std::uint8_t>(modifiers))
        );
    }

    void pushKeybind(lua_State* L, geode::Keybind const& keybind) {
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, static_cast<int>(keybind.key));
        lua_setfield(L, -2, "key");
        lua_pushinteger(L, static_cast<int>(keybind.modifiers));
        lua_setfield(L, -2, "modifiers");
    }

    int kbFromString(lua_State* L) {
        auto str = check<std::string>(L, 1, "geode.Keybind.fromString");
        auto result = geode::Keybind::fromString(str);
        if (result.isErr()) {
            return pushNilErr(L, result.unwrapErr());
        }
        pushKeybind(L, result.unwrap());
        return 1;
    }

    int kbToString(lua_State* L) {
        auto keybind = buildKeybind(L, 1, "geode.Keybind.toString");
        push(L, keybind.toString());
        return 1;
    }

    int kbCreateNode(lua_State* L) {
        auto keybind = buildKeybind(L, 1, "geode.Keybind.createNode");
        auto* node = keybind.createNode();
        if (!node) {
            lua_pushnil(L);
            return 1;
        }
        Usertype<cocos2d::CCNode>::pushOwned(L, node);
        return 1;
    }
#endif
} // namespace

namespace luax {
#ifndef LUAUAPI_HOST_TESTS
    geode::Result<void> registerGeodeBase64(lua_State* L) {
        getOrCreateTable(L, "geode.utils.base64");
        setTableCFunction(L, -1, "encode", &b64Encode);
        setTableCFunction(L, -1, "decode", &b64Decode);
        setTableCFunction(L, -1, "decodeString", &b64DecodeString);

        lua_createtable(L, 0, 4);
        setIntField(L, "Normal", static_cast<int>(base64::Base64Variant::Normal));
        setIntField(L, "NormalNoPad", static_cast<int>(base64::Base64Variant::NormalNoPad));
        setIntField(L, "Url", static_cast<int>(base64::Base64Variant::Url));
        setIntField(L, "UrlWithPad", static_cast<int>(base64::Base64Variant::UrlWithPad));
        lua_setfield(L, -2, "Variant");

        lua_pop(L, 1);
        return geode::Ok();
    }
#endif

    geode::Result<void> registerGeodeJson(lua_State* L) {
        luax::getOrCreateTable(L, "geode");
        lua_newtable(L);
        setTableCFunction(L, -1, "parse", &jsonParse);
        setTableCFunction(L, -1, "dump", &jsonDump);
        lua_setfield(L, -2, "json");
        lua_pop(L, 1);
        return geode::Ok();
    }

#ifndef LUAUAPI_HOST_TESTS
    geode::Result<void> registerGeodeVersion(lua_State* L) {
        getOrCreateTable(L, "geode.VersionInfo");
        setTableCFunction(L, -1, "parse", &viParse);
        setTableCFunction(L, -1, "compare", &viCompare);
        setTableCFunction(L, -1, "matches", &viMatches);
        lua_pop(L, 1);
        return geode::Ok();
    }

    geode::Result<void> registerGeodePermission(lua_State* L) {
        getOrCreateTable(L, "geode.utils.permission");
        setTableCFunction(L, -1, "getPermissionStatus", &permGetStatus);
        setTableCFunction(L, -1, "requestPermission", &permRequest);

        lua_createtable(L, 0, 2);
        setIntField(L, "ReadAllFiles", static_cast<int>(permission::Permission::ReadAllFiles));
        setIntField(L, "RecordAudio", static_cast<int>(permission::Permission::RecordAudio));
        lua_setfield(L, -2, "Permission");

        lua_pop(L, 1);
        return geode::Ok();
    }

    geode::Result<void> registerGeodeColorProvider(lua_State* L) {
        getOrCreateTable(L, "geode.ColorProvider");
        setTableCFunction(L, -1, "define", &cpDefine);
        setTableCFunction(L, -1, "override", &cpOverride);
        setTableCFunction(L, -1, "reset", &cpReset);
        setTableCFunction(L, -1, "color", &cpColor);
        setTableCFunction(L, -1, "color3b", &cpColor3b);
        lua_pop(L, 1);
        return geode::Ok();
    }

    geode::Result<void> registerGeodeKeybind(lua_State* L) {
        getOrCreateTable(L, "geode.Keybind");
        setTableCFunction(L, -1, "fromString", &kbFromString);
        setTableCFunction(L, -1, "toString", &kbToString);
        setTableCFunction(L, -1, "createNode", &kbCreateNode);
        lua_pop(L, 1);
        return geode::Ok();
    }
#endif
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_base64_lib, registerGeodeBase64)
LUAX_BINDING(geode_json_lib, registerGeodeJson)
LUAX_BINDING(geode_version_lib, registerGeodeVersion)
LUAX_BINDING(geode_permission_lib, registerGeodePermission)
LUAX_BINDING(geode_color_provider_lib, registerGeodeColorProvider)
LUAX_BINDING(geode_keybind_lib, registerGeodeKeybind)
#endif
