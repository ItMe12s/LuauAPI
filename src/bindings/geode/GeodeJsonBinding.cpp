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

namespace {
    using namespace luax;

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
} // namespace

namespace luax {
    geode::Result<void> registerGeodeJson(lua_State* L) {
        luax::getOrCreateTable(L, "geode");
        lua_newtable(L);
        setTableCFunction(L, -1, "parse", &jsonParse);
        setTableCFunction(L, -1, "dump", &jsonDump);
        lua_setfield(L, -2, "json");
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_json_lib, registerGeodeJson)
#endif
