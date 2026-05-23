#include "../Binding.hpp"
#include "internal/Ref.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Types.hpp"
#include "internal/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <string>

namespace {
    using namespace luax;
    using Label = cocos2d::CCLabelBMFont;

    int label_create(lua_State* L) {
        assertMainThread();
        auto text = check<std::string>(L, 1, "CCLabelBMFont:create");
        auto font = check<std::string>(L, 2, "CCLabelBMFont:create");
        Usertype<Label>::pushOwned(L, Label::create(text.c_str(), font.c_str()));
        return 1;
    }

    int label_setString(lua_State* L) {
        auto self = Usertype<Label>::check(L, 1, "CCLabelBMFont:setString");
        auto text = check<std::string>(L, 2, "CCLabelBMFont:setString");
        assertMainThread();
        self->setString(text.c_str());
        return 0;
    }

    int label_getString(lua_State* L) {
        auto self = Usertype<Label>::check(L, 1, "CCLabelBMFont:getString");
        push(L, std::string(self->getString()));
        return 1;
    }

    int label_setColor(lua_State* L) {
        auto self = Usertype<Label>::check(L, 1, "CCLabelBMFont:setColor");
        assertMainThread();
        if (lua_type(L, 2) != LUA_TTABLE) {
            luaL_error(L, "CCLabelBMFont:setColor expected color table");
        }
        self->setColor(toColor3B(L, 2, "CCLabelBMFont:setColor"));
        return 0;
    }

    void bindCCLabelBMFont(lua_State* L) {
        Usertype<Label>::registerType(L, "CCLabelBMFont", { Usertype<cocos2d::CCNode>::tag() });
        Usertype<Label>::method(L, "setString", &label_setString);
        Usertype<Label>::method(L, "getString", &label_getString);
        Usertype<Label>::method(L, "setColor",  &label_setColor);

        getOrCreateTable(L, "geode.cocos2d");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &label_create, "CCLabelBMFont.create");
        lua_setfield(L, -2, "create");
        lua_setfield(L, -2, "CCLabelBMFont");
        lua_pop(L, 1);
    }

    LUAX_BINDING(CCLabelBMFont, bindCCLabelBMFont)
}
