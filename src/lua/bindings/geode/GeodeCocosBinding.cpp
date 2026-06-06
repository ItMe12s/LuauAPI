#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/framework/Types.hpp"
#include "lua/bindings/framework/Usertype.hpp"

#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <lualib.h>
#include <string>

namespace {
    using namespace luax;

    bool optBool(lua_State* L, int idx, bool def) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return def;
        return check<bool>(L, idx, "geode.cocos");
    }

    int cocosInvert3B(lua_State* L) {
        auto color = check<cocos2d::ccColor3B>(L, 1, "geode.cocos.invert3B");
        push(L, geode::cocos::invert3B(color));
        return 1;
    }

    int cocosInvert4B(lua_State* L) {
        auto color = check<cocos2d::ccColor4B>(L, 1, "geode.cocos.invert4B");
        push(L, geode::cocos::invert4B(color));
        return 1;
    }

    int cocosLighten3B(lua_State* L) {
        auto color = check<cocos2d::ccColor3B>(L, 1, "geode.cocos.lighten3B");
        auto amount = check<int>(L, 2, "geode.cocos.lighten3B");
        push(L, geode::cocos::lighten3B(color, amount));
        return 1;
    }

    int cocosDarken3B(lua_State* L) {
        auto color = check<cocos2d::ccColor3B>(L, 1, "geode.cocos.darken3B");
        auto amount = check<int>(L, 2, "geode.cocos.darken3B");
        push(L, geode::cocos::darken3B(color, amount));
        return 1;
    }

    int cocosTo3B(lua_State* L) {
        auto color = check<cocos2d::ccColor4B>(L, 1, "geode.cocos.to3B");
        push(L, geode::cocos::to3B(color));
        return 1;
    }

    int cocosTo4B(lua_State* L) {
        auto color = check<cocos2d::ccColor3B>(L, 1, "geode.cocos.to4B");
        int alpha = 255;
        if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
            alpha = check<int>(L, 2, "geode.cocos.to4B");
        }
        push(L, geode::cocos::to4B(color, static_cast<GLubyte>(alpha)));
        return 1;
    }

    int cocosTo4F(lua_State* L) {
        auto color = check<cocos2d::ccColor4B>(L, 1, "geode.cocos.to4F");
        push(L, geode::cocos::to4F(color));
        return 1;
    }

    int cocosDrawColor4B(lua_State* L) {
        auto color = check<cocos2d::ccColor4B>(L, 1, "geode.cocos.ccDrawColor4B");
        geode::cocos::ccDrawColor4B(color);
        return 0;
    }

    int cocosCc3bFromHexString(lua_State* L) {
        auto hex = check<std::string>(L, 1, "geode.cocos.cc3bFromHexString");
        bool permissive = optBool(L, 2, false);
        auto result = geode::cocos::cc3bFromHexString(hex, permissive);
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
        }
        push(L, result.unwrap());
        return 1;
    }

    int cocosCc4bFromHexString(lua_State* L) {
        auto hex = check<std::string>(L, 1, "geode.cocos.cc4bFromHexString");
        bool requireAlpha = optBool(L, 2, false);
        bool permissive = optBool(L, 3, false);
        auto result = geode::cocos::cc4bFromHexString(hex, requireAlpha, permissive);
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
        }
        push(L, result.unwrap());
        return 1;
    }

    int cocosGetObjectName(lua_State* L) {
        auto* obj = Usertype<cocos2d::CCObject>::check(L, 1, "geode.cocos.getObjectName");
        push(L, std::string(geode::cocos::getObjectName(obj)));
        return 1;
    }

    int cocosHandleTouchPriority(lua_State* L) {
        auto* node = Usertype<cocos2d::CCNode>::check(L, 1, "geode.cocos.handleTouchPriority");
        bool force = optBool(L, 2, false);
        geode::cocos::handleTouchPriority(node, force);
        return 0;
    }

    int cocosHandleTouchPriorityWith(lua_State* L) {
        auto* node = Usertype<cocos2d::CCNode>::check(L, 1, "geode.cocos.handleTouchPriorityWith");
        auto priority = check<int>(L, 2, "geode.cocos.handleTouchPriorityWith");
        bool force = optBool(L, 3, false);
        geode::cocos::handleTouchPriorityWith(node, priority, force);
        return 0;
    }
} // namespace

namespace luax {
    geode::Result<void> registerGeodeCocos(lua_State* L) {
        getOrCreateTable(L, "geode.cocos");
        setTableCFunction(L, -1, "invert3B", &cocosInvert3B);
        setTableCFunction(L, -1, "invert4B", &cocosInvert4B);
        setTableCFunction(L, -1, "lighten3B", &cocosLighten3B);
        setTableCFunction(L, -1, "darken3B", &cocosDarken3B);
        setTableCFunction(L, -1, "to3B", &cocosTo3B);
        setTableCFunction(L, -1, "to4B", &cocosTo4B);
        setTableCFunction(L, -1, "to4F", &cocosTo4F);
        setTableCFunction(L, -1, "ccDrawColor4B", &cocosDrawColor4B);
        setTableCFunction(L, -1, "cc3bFromHexString", &cocosCc3bFromHexString);
        setTableCFunction(L, -1, "cc4bFromHexString", &cocosCc4bFromHexString);
        setTableCFunction(L, -1, "getObjectName", &cocosGetObjectName);
        setTableCFunction(L, -1, "handleTouchPriority", &cocosHandleTouchPriority);
        setTableCFunction(L, -1, "handleTouchPriorityWith", &cocosHandleTouchPriorityWith);
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_cocos_lib, registerGeodeCocos)
#endif
