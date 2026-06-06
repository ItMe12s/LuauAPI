#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/framework/Types.hpp"

#include <Geode/utils/ColorProvider.hpp>
#include <lua.h>
#include <lualib.h>
#include <string>

namespace {
    using namespace luax;

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
} // namespace

namespace luax {
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
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_color_provider_lib, registerGeodeColorProvider)
#endif
