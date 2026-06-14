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
