#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"

#include <Geode/utils/VersionInfo.hpp>
#include <lua.h>
#include <lualib.h>
#include <string>

namespace {
    using namespace luax;

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
} // namespace

namespace luax {
    geode::Result<void> registerGeodeVersion(lua_State* L) {
        getOrCreateTable(L, "geode.VersionInfo");
        setTableCFunction(L, -1, "parse", &viParse);
        setTableCFunction(L, -1, "compare", &viCompare);
        setTableCFunction(L, -1, "matches", &viMatches);
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_version_lib, registerGeodeVersion)
#endif
