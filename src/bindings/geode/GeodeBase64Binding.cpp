#include "framework/Binding.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <Geode/utils/base64.hpp>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <string_view>

namespace {
    using namespace luax;
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
} // namespace

namespace luax {
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
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_base64_lib, registerGeodeBase64)
#endif
