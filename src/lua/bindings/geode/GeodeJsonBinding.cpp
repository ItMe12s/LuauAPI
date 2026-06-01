#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/JsonConvert.hpp"

#include <matjson.hpp>

#include <lua.h>
#include <lualib.h>

#include <string>

namespace {
    using namespace luax;

    int jsonParse(lua_State* L) {
        auto text = check<std::string>(L, 1, "geode.json.parse");
        auto result = matjson::Value::parse(text);
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
        }
        pushJson(L, result.unwrap());
        return 1;
    }

    int jsonDump(lua_State* L) {
        int indent = matjson::NO_INDENTATION;
        if (!lua_isnoneornil(L, 2)) {
            indent = static_cast<int>(luaL_checkinteger(L, 2));
        }
        auto value = toJson(L, 1);
        push(L, value.dump(indent));
        return 1;
    }

    geode::Result<void> registerGeodeJson(lua_State* L) {
        luax::getOrCreateTable(L, "geode");
        lua_newtable(L);
        setTableCFunction(L, -1, "parse", &jsonParse);
        setTableCFunction(L, -1, "dump", &jsonDump);
        lua_setfield(L, -2, "json");
        lua_pop(L, 1);
        return geode::Ok();
    }
}

LUAX_BINDING(geode_json_lib, registerGeodeJson)
