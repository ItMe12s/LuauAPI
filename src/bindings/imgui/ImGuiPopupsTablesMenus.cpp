#include "bindings/imgui/ImGuiBindingInternal.hpp"

#include <imgui.h>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;

    int imguiTabBar(lua_State* L) {
        requireFrame(L, "imgui.tabBar");
        char const* id = checkLuaString(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImGuiTabBarFlags flags = 0;
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiTabBarFlags>(optFieldNumber(L, 3, "flags", 0.0f));
        }

        bool visible = ImGui::BeginTabBar(id, flags);
        ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::TabBar, visible};
        if (visible) callDrawClosure(L, 2, "imgui.tabBar");
        return 0;
    }

    int imguiTabItem(lua_State* L) {
        requireFrame(L, "imgui.tabItem");
        char const* label = checkLuaString(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImGuiTabItemFlags flags = 0;
        bool closable = false;
        bool open = true;
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiTabItemFlags>(optFieldNumber(L, 3, "flags", 0.0f));
            closable = optFieldBool(L, 3, "closable", false);
        }

        bool visible = ImGui::BeginTabItem(label, closable ? &open : nullptr, flags);
        ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::TabItem, visible};
        if (visible) callDrawClosure(L, 2, "imgui.tabItem");

        if (closable) {
            lua_pushboolean(L, open);
            return 1;
        }
        return 0;
    }

    int imguiOpenPopup(lua_State* L) {
        requireFrame(L, "imgui.openPopup");
        char const* id = checkLuaString(L, 1);
        ImGuiPopupFlags flags = ImGuiPopupFlags_None;
        if (!lua_isnoneornil(L, 2)) {
            flags = static_cast<ImGuiPopupFlags>(check<int>(L, 2, "imgui.openPopup"));
        }
        ImGui::OpenPopup(id, flags);
        return 0;
    }

    int imguiCloseCurrentPopup(lua_State* L) {
        requireFrame(L, "imgui.closeCurrentPopup");
        ImGui::CloseCurrentPopup();
        return 0;
    }

    int imguiPopup(lua_State* L) {
        requireFrame(L, "imgui.popup");
        char const* id = checkLuaString(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImGuiWindowFlags flags = 0;
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiWindowFlags>(optFieldNumber(L, 3, "flags", 0.0f));
        }

        bool visible = ImGui::BeginPopup(id, flags);
        ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::Popup, visible};
        if (visible) callDrawClosure(L, 2, "imgui.popup");
        lua_pushboolean(L, visible);
        return 1;
    }

    int imguiPopupModal(lua_State* L) {
        requireFrame(L, "imgui.popupModal");
        char const* title = checkLuaString(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImGuiWindowFlags flags = 0;
        bool closable = false;
        bool open = true;
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiWindowFlags>(optFieldNumber(L, 3, "flags", 0.0f));
            closable = optFieldBool(L, 3, "closable", false);
        }

        bool visible = ImGui::BeginPopupModal(title, closable ? &open : nullptr, flags);
        ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::Popup, visible};
        if (visible) callDrawClosure(L, 2, "imgui.popupModal");

        if (closable) {
            lua_pushboolean(L, open);
            return 1;
        }
        return 0;
    }

    int imguiSetTooltip(lua_State* L) {
        requireFrame(L, "imgui.setTooltip");
        size_t textLen = 0;
        char const* text = checkLuaString(L, 1, &textLen);
        ImGui::SetTooltip("%.*s", static_cast<int>(textLen), text);
        return 0;
    }

    int imguiTooltip(lua_State* L) {
        requireFrame(L, "imgui.tooltip");
        luaL_checktype(L, 1, LUA_TFUNCTION);
        if (!ImGui::IsItemHovered()) {
            return 0;
        }
        ImGuiTooltipGuard tooltipGuard;
        callDrawClosure(L, 1, "imgui.tooltip");
        return 0;
    }
} // namespace

namespace luax {
    void registerImGuiPopups(lua_State* L) {
        setTableCFunction(L, -1, "tabBar", &imguiTabBar);
        setTableCFunction(L, -1, "tabItem", &imguiTabItem);
        setTableCFunction(L, -1, "openPopup", &imguiOpenPopup);
        setTableCFunction(L, -1, "closeCurrentPopup", &imguiCloseCurrentPopup);
        setTableCFunction(L, -1, "popup", &imguiPopup);
        setTableCFunction(L, -1, "popupModal", &imguiPopupModal);
        setTableCFunction(L, -1, "setTooltip", &imguiSetTooltip);
        setTableCFunction(L, -1, "tooltip", &imguiTooltip);
    }
} // namespace luax

#include <imgui.h>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;

    int imguiTable(lua_State* L) {
        requireFrame(L, "imgui.table");
        char const* id = checkLuaString(L, 1);
        int columns = check<int>(L, 2, "imgui.table");
        luaL_checktype(L, 3, LUA_TFUNCTION);

        ImGuiTableFlags flags = 0;
        ImVec2 outerSize(0.0f, 0.0f);
        float innerWidth = 0.0f;
        if (lua_istable(L, 4)) {
            flags = static_cast<ImGuiTableFlags>(optFieldNumber(L, 4, "flags", 0.0f));
            optFieldVec2(L, 4, "outerSize", outerSize, "imgui.table");
            innerWidth = optFieldNumber(L, 4, "innerWidth", 0.0f);
        }

        bool visible = ImGui::BeginTable(id, columns, flags, outerSize, innerWidth);
        ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::Table, visible};
        if (visible) callDrawClosure(L, 3, "imgui.table");
        return 0;
    }

    int imguiTableNextRow(lua_State* L) {
        requireFrame(L, "imgui.tableNextRow");
        ImGuiTableRowFlags flags = 0;
        if (!lua_isnoneornil(L, 1)) {
            flags = static_cast<ImGuiTableRowFlags>(check<int>(L, 1, "imgui.tableNextRow"));
        }
        ImGui::TableNextRow(flags);
        return 0;
    }

    int imguiTableNextColumn(lua_State* L) {
        requireFrame(L, "imgui.tableNextColumn");
        lua_pushboolean(L, ImGui::TableNextColumn());
        return 1;
    }

    int imguiTableSetColumnIndex(lua_State* L) {
        requireFrame(L, "imgui.tableSetColumnIndex");
        int column = check<int>(L, 1, "imgui.tableSetColumnIndex");
        lua_pushboolean(L, ImGui::TableSetColumnIndex(column));
        return 1;
    }

    int imguiTableSetupColumn(lua_State* L) {
        requireFrame(L, "imgui.tableSetupColumn");
        char const* label = checkLuaString(L, 1);

        ImGuiTableColumnFlags flags = 0;
        float width = 0.0f;
        ImGuiID userId = 0;
        if (lua_istable(L, 2)) {
            flags = static_cast<ImGuiTableColumnFlags>(optFieldNumber(L, 2, "flags", 0.0f));
            width = optFieldNumber(L, 2, "width", 0.0f);
            userId = static_cast<ImGuiID>(optFieldNumber(L, 2, "userId", 0.0f));
        }

        ImGui::TableSetupColumn(label, flags, width, userId);
        return 0;
    }

    int imguiTableHeadersRow(lua_State* L) {
        requireFrame(L, "imgui.tableHeadersRow");
        ImGui::TableHeadersRow();
        return 0;
    }
} // namespace

namespace luax {
    void registerImGuiTables(lua_State* L) {
        setTableCFunction(L, -1, "table", &imguiTable);
        setTableCFunction(L, -1, "tableNextRow", &imguiTableNextRow);
        setTableCFunction(L, -1, "tableNextColumn", &imguiTableNextColumn);
        setTableCFunction(L, -1, "tableSetColumnIndex", &imguiTableSetColumnIndex);
        setTableCFunction(L, -1, "tableSetupColumn", &imguiTableSetupColumn);
        setTableCFunction(L, -1, "tableHeadersRow", &imguiTableHeadersRow);
    }
} // namespace luax

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
