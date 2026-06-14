#include "bindings/imgui/ImGuiBindingInternal.hpp"

#include <imgui.h>
#include <lua.h>
#include <lualib.h>
#include <vector>

namespace {
    using namespace luax;

    std::vector<char const*> toCStringView(std::vector<std::string> const& items) {
        std::vector<char const*> ptrs;
        ptrs.reserve(items.size());
        for (auto const& item : items) {
            ptrs.push_back(item.c_str());
        }
        return ptrs;
    }

    int imguiCombo(lua_State* L) {
        requireFrame(L, "imgui.combo");
        char const* label = checkLuaString(L, 1);
        int index = check<int>(L, 2, "imgui.combo");
        auto items = readStringArray(L, 3, "imgui.combo");
        if (items.empty()) {
            luaL_error(L, "imgui.combo: items must not be empty");
        }
        if (index < 0 || index >= static_cast<int>(items.size())) {
            index = 0;
        }

        ImGuiComboFlags flags = 0;
        int popupMaxHeight = -1;
        if (lua_istable(L, 4)) {
            flags = static_cast<ImGuiComboFlags>(optFieldNumber(L, 4, "flags", 0.0f));
            popupMaxHeight = static_cast<int>(optFieldNumber(L, 4, "height", -1.0f));
        }

        auto ptrs = toCStringView(items);
        if (flags != 0) {
            if (!ImGui::BeginCombo(label, ptrs[static_cast<std::size_t>(index)], flags)) {
                lua_pushinteger(L, index);
                return 1;
            }
            ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::Combo, true};
            for (int i = 0; i < static_cast<int>(ptrs.size()); ++i) {
                bool selected = i == index;
                if (ImGui::Selectable(ptrs[static_cast<std::size_t>(i)], selected)) {
                    index = i;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        else {
            ImGui::Combo(label, &index, ptrs.data(), static_cast<int>(ptrs.size()), popupMaxHeight);
        }
        lua_pushinteger(L, index);
        return 1;
    }

    int imguiComboPopup(lua_State* L) {
        requireFrame(L, "imgui.comboPopup");
        char const* label = checkLuaString(L, 1);
        char const* preview = checkLuaString(L, 2);
        luaL_checktype(L, 3, LUA_TFUNCTION);

        ImGuiComboFlags flags = 0;
        if (lua_istable(L, 4)) {
            flags = static_cast<ImGuiComboFlags>(optFieldNumber(L, 4, "flags", 0.0f));
        }

        bool open = ImGui::BeginCombo(label, preview, flags);
        ImGuiConditionalEndGuard endGuard{ImGuiConditionalEndGuard::Kind::Combo, open};
        if (open) callDrawClosure(L, 3, "imgui.comboPopup");
        lua_pushboolean(L, open);
        return 1;
    }

    int imguiSelectable(lua_State* L) {
        requireFrame(L, "imgui.selectable");
        char const* label = checkLuaString(L, 1);
        bool selected = check<bool>(L, 2, "imgui.selectable");

        ImGuiSelectableFlags flags = 0;
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiSelectableFlags>(optFieldNumber(L, 3, "flags", 0.0f));
        }

        ImGui::Selectable(label, &selected, flags);
        lua_pushboolean(L, selected);
        return 1;
    }

    int imguiListBox(lua_State* L) {
        requireFrame(L, "imgui.listBox");
        char const* label = checkLuaString(L, 1);
        int index = check<int>(L, 2, "imgui.listBox");
        auto items = readStringArray(L, 3, "imgui.listBox");
        if (items.empty()) {
            luaL_error(L, "imgui.listBox: items must not be empty");
        }

        int height = -1;
        if (lua_istable(L, 4)) {
            height = static_cast<int>(optFieldNumber(L, 4, "height", -1.0f));
        }

        auto ptrs = toCStringView(items);
        ImGui::ListBox(label, &index, ptrs.data(), static_cast<int>(ptrs.size()), height);
        lua_pushinteger(L, index);
        return 1;
    }

    int imguiRadioButton(lua_State* L) {
        requireFrame(L, "imgui.radioButton");
        char const* label = checkLuaString(L, 1);
        bool active = check<bool>(L, 2, "imgui.radioButton");
        lua_pushboolean(L, ImGui::RadioButton(label, active));
        return 1;
    }

    int imguiDragFloat(lua_State* L) {
        requireFrame(L, "imgui.dragFloat");
        char const* label = checkLuaString(L, 1);
        float value = check<float>(L, 2, "imgui.dragFloat");

        float speed = 1.0f;
        float vmin = 0.0f;
        float vmax = 0.0f;
        char const* fmt = "%.3f";
        ImGuiSliderFlags flags = 0;
        if (lua_istable(L, 3)) {
            speed = optFieldNumber(L, 3, "speed", 1.0f);
            vmin = optFieldNumber(L, 3, "min", 0.0f);
            vmax = optFieldNumber(L, 3, "max", 0.0f);
            lua_getfield(L, 3, "fmt");
            if (lua_isstring(L, -1)) {
                fmt = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            flags = static_cast<ImGuiSliderFlags>(optFieldNumber(L, 3, "flags", 0.0f));
        }

        ImGui::DragFloat(label, &value, speed, vmin, vmax, fmt, flags);
        lua_pushnumber(L, value);
        return 1;
    }

    int imguiDragInt(lua_State* L) {
        requireFrame(L, "imgui.dragInt");
        char const* label = checkLuaString(L, 1);
        int value = check<int>(L, 2, "imgui.dragInt");

        float speed = 1.0f;
        int vmin = 0;
        int vmax = 0;
        char const* fmt = "%d";
        ImGuiSliderFlags flags = 0;
        if (lua_istable(L, 3)) {
            speed = optFieldNumber(L, 3, "speed", 1.0f);
            vmin = static_cast<int>(optFieldNumber(L, 3, "min", 0.0f));
            vmax = static_cast<int>(optFieldNumber(L, 3, "max", 0.0f));
            lua_getfield(L, 3, "fmt");
            if (lua_isstring(L, -1)) {
                fmt = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            flags = static_cast<ImGuiSliderFlags>(optFieldNumber(L, 3, "flags", 0.0f));
        }

        ImGui::DragInt(label, &value, speed, vmin, vmax, fmt, flags);
        lua_pushinteger(L, value);
        return 1;
    }

    int imguiColorEdit3(lua_State* L) {
        requireFrame(L, "imgui.colorEdit3");
        char const* label = checkLuaString(L, 1);
        float color[3] = {
            fieldNumber(L, 2, "x", "imgui.colorEdit3"),
            fieldNumber(L, 2, "y", "imgui.colorEdit3"),
            fieldNumber(L, 2, "z", "imgui.colorEdit3"),
        };

        ImGuiColorEditFlags flags = 0;
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiColorEditFlags>(optFieldNumber(L, 3, "flags", 0.0f));
        }

        ImGui::ColorEdit3(label, color, flags);
        pushImVec3(L, color[0], color[1], color[2]);
        return 1;
    }

    int imguiColorEdit4(lua_State* L) {
        requireFrame(L, "imgui.colorEdit4");
        char const* label = checkLuaString(L, 1);
        ImVec4 color = toImVec4(L, 2, "imgui.colorEdit4");

        ImGuiColorEditFlags flags = 0;
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiColorEditFlags>(optFieldNumber(L, 3, "flags", 0.0f));
        }

        ImGui::ColorEdit4(label, &color.x, flags);
        pushImVec4(L, color);
        return 1;
    }

    int imguiColorButton(lua_State* L) {
        requireFrame(L, "imgui.colorButton");
        char const* label = checkLuaString(L, 1);
        ImVec4 color = toImVec4(L, 2, "imgui.colorButton");

        ImGuiColorEditFlags flags = 0;
        ImVec2 size(-1.0f, -1.0f);
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiColorEditFlags>(optFieldNumber(L, 3, "flags", 0.0f));
            optFieldVec2(L, 3, "size", size, "imgui.colorButton");
        }

        lua_pushboolean(L, ImGui::ColorButton(label, color, flags, size));
        return 1;
    }

    int imguiIsItemHovered(lua_State* L) {
        requireFrame(L, "imgui.isItemHovered");
        ImGuiHoveredFlags flags = ImGuiHoveredFlags_None;
        if (!lua_isnoneornil(L, 1)) {
            flags = static_cast<ImGuiHoveredFlags>(check<int>(L, 1, "imgui.isItemHovered"));
        }
        lua_pushboolean(L, ImGui::IsItemHovered(flags));
        return 1;
    }

    int imguiIsItemClicked(lua_State* L) {
        requireFrame(L, "imgui.isItemClicked");
        ImGuiMouseButton button = ImGuiMouseButton_Left;
        if (!lua_isnoneornil(L, 1)) {
            button = static_cast<ImGuiMouseButton>(check<int>(L, 1, "imgui.isItemClicked"));
        }
        lua_pushboolean(L, ImGui::IsItemClicked(button));
        return 1;
    }

    int imguiIsWindowHovered(lua_State* L) {
        requireFrame(L, "imgui.isWindowHovered");
        ImGuiHoveredFlags flags = ImGuiHoveredFlags_None;
        if (!lua_isnoneornil(L, 1)) {
            flags = static_cast<ImGuiHoveredFlags>(check<int>(L, 1, "imgui.isWindowHovered"));
        }
        lua_pushboolean(L, ImGui::IsWindowHovered(flags));
        return 1;
    }

    int imguiSetNextWindowFocus(lua_State* L) {
        requireFrame(L, "imgui.setNextWindowFocus");
        ImGui::SetNextWindowFocus();
        return 0;
    }

    int imguiSetWindowFocus(lua_State* L) {
        requireFrame(L, "imgui.setWindowFocus");
        if (lua_isnoneornil(L, 1)) {
            ImGui::SetWindowFocus();
        }
        else {
            ImGui::SetWindowFocus(checkLuaString(L, 1));
        }
        return 0;
    }
} // namespace

namespace luax {
    void registerImGuiWidgets(lua_State* L) {
        setTableCFunction(L, -1, "combo", &imguiCombo);
        setTableCFunction(L, -1, "comboPopup", &imguiComboPopup);
        setTableCFunction(L, -1, "selectable", &imguiSelectable);
        setTableCFunction(L, -1, "listBox", &imguiListBox);
        setTableCFunction(L, -1, "radioButton", &imguiRadioButton);
        setTableCFunction(L, -1, "dragFloat", &imguiDragFloat);
        setTableCFunction(L, -1, "dragInt", &imguiDragInt);
        setTableCFunction(L, -1, "colorEdit3", &imguiColorEdit3);
        setTableCFunction(L, -1, "colorEdit4", &imguiColorEdit4);
        setTableCFunction(L, -1, "colorButton", &imguiColorButton);
        setTableCFunction(L, -1, "isItemHovered", &imguiIsItemHovered);
        setTableCFunction(L, -1, "isItemClicked", &imguiIsItemClicked);
        setTableCFunction(L, -1, "isWindowHovered", &imguiIsWindowHovered);
        setTableCFunction(L, -1, "setNextWindowFocus", &imguiSetNextWindowFocus);
        setTableCFunction(L, -1, "setWindowFocus", &imguiSetWindowFocus);
    }
} // namespace luax
