#include "bindings/imgui/ImGuiBindingInternal.hpp"

#include <imgui.h>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;

    int imguiMenuBar(lua_State* L) {
        requireFrame(L, "imgui.menuBar");
        luaL_checktype(L, 1, LUA_TFUNCTION);

        bool visible = ImGui::BeginMenuBar();
        ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::MenuBar, visible};
        if (visible) callDrawClosure(L, 1, "imgui.menuBar");
        return 0;
    }

    int imguiMenu(lua_State* L) {
        requireFrame(L, "imgui.menu");
        char const* label = checkLuaString(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        bool enabled = true;
        if (lua_istable(L, 3)) {
            enabled = optFieldBool(L, 3, "enabled", true);
        }

        bool visible = ImGui::BeginMenu(label, enabled);
        ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::Menu, visible};
        if (visible) callDrawClosure(L, 2, "imgui.menu");
        return 0;
    }

    int imguiMenuItem(lua_State* L) {
        requireFrame(L, "imgui.menuItem");
        char const* label = checkLuaString(L, 1);

        bool trackSelected = !lua_isnoneornil(L, 2) && lua_isboolean(L, 2);
        bool selected = trackSelected ? check<bool>(L, 2, "imgui.menuItem") : false;

        char const* shortcut = nullptr;
        bool enabled = true;
        int optsIdx = trackSelected ? 3 : 2;
        if (lua_istable(L, optsIdx)) {
            lua_getfield(L, optsIdx, "shortcut");
            if (lua_isstring(L, -1)) {
                shortcut = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            enabled = optFieldBool(L, optsIdx, "enabled", true);
        }

        bool clicked = trackSelected ? ImGui::MenuItem(label, shortcut, &selected, enabled) :
                                       ImGui::MenuItem(label, shortcut, nullptr, enabled);

        if (trackSelected) {
            lua_pushboolean(L, selected);
            return 1;
        }
        lua_pushboolean(L, clicked);
        return 1;
    }
} // namespace

namespace luax {
    void registerImGuiMenus(lua_State* L) {
        setTableCFunction(L, -1, "menuBar", &imguiMenuBar);
        setTableCFunction(L, -1, "menu", &imguiMenu);
        setTableCFunction(L, -1, "menuItem", &imguiMenuItem);
    }
} // namespace luax
