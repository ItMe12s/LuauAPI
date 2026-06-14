#include "bindings/imgui/ImGuiBindingInternal.hpp"

#include <cstring>
#include <imgui.h>
#include <lua.h>
#include <lualib.h>
#include <optional>

namespace {
    using namespace luax;

    struct FloatStyleField {
        char const* key;
        ImGuiStyleVar var;
    };

    struct Vec2StyleField {
        char const* key;
        ImGuiStyleVar var;
    };

    constexpr FloatStyleField kFloatStyleFields[] = {
        {"alpha", ImGuiStyleVar_Alpha},
        {"disabledAlpha", ImGuiStyleVar_DisabledAlpha},
        {"windowRounding", ImGuiStyleVar_WindowRounding},
        {"windowBorderSize", ImGuiStyleVar_WindowBorderSize},
        {"childRounding", ImGuiStyleVar_ChildRounding},
        {"childBorderSize", ImGuiStyleVar_ChildBorderSize},
        {"popupRounding", ImGuiStyleVar_PopupRounding},
        {"popupBorderSize", ImGuiStyleVar_PopupBorderSize},
        {"frameRounding", ImGuiStyleVar_FrameRounding},
        {"frameBorderSize", ImGuiStyleVar_FrameBorderSize},
        {"indentSpacing", ImGuiStyleVar_IndentSpacing},
        {"scrollbarSize", ImGuiStyleVar_ScrollbarSize},
        {"scrollbarRounding", ImGuiStyleVar_ScrollbarRounding},
        {"scrollbarPadding", ImGuiStyleVar_ScrollbarPadding},
        {"grabMinSize", ImGuiStyleVar_GrabMinSize},
        {"grabRounding", ImGuiStyleVar_GrabRounding},
        {"imageRounding", ImGuiStyleVar_ImageRounding},
        {"imageBorderSize", ImGuiStyleVar_ImageBorderSize},
        {"tabRounding", ImGuiStyleVar_TabRounding},
        {"tabBorderSize", ImGuiStyleVar_TabBorderSize},
        {"tabMinWidthBase", ImGuiStyleVar_TabMinWidthBase},
        {"tabMinWidthShrink", ImGuiStyleVar_TabMinWidthShrink},
        {"tabBarBorderSize", ImGuiStyleVar_TabBarBorderSize},
        {"tabBarOverlineSize", ImGuiStyleVar_TabBarOverlineSize},
        {"tableAngledHeadersAngle", ImGuiStyleVar_TableAngledHeadersAngle},
        {"treeLinesSize", ImGuiStyleVar_TreeLinesSize},
        {"treeLinesRounding", ImGuiStyleVar_TreeLinesRounding},
        {"dragDropTargetRounding", ImGuiStyleVar_DragDropTargetRounding},
        {"separatorSize", ImGuiStyleVar_SeparatorSize},
        {"separatorTextBorderSize", ImGuiStyleVar_SeparatorTextBorderSize},
    };

    constexpr Vec2StyleField kVec2StyleFields[] = {
        {"windowPadding", ImGuiStyleVar_WindowPadding},
        {"windowMinSize", ImGuiStyleVar_WindowMinSize},
        {"windowTitleAlign", ImGuiStyleVar_WindowTitleAlign},
        {"framePadding", ImGuiStyleVar_FramePadding},
        {"itemSpacing", ImGuiStyleVar_ItemSpacing},
        {"itemInnerSpacing", ImGuiStyleVar_ItemInnerSpacing},
        {"cellPadding", ImGuiStyleVar_CellPadding},
        {"buttonTextAlign", ImGuiStyleVar_ButtonTextAlign},
        {"selectableTextAlign", ImGuiStyleVar_SelectableTextAlign},
        {"separatorTextAlign", ImGuiStyleVar_SeparatorTextAlign},
        {"separatorTextPadding", ImGuiStyleVar_SeparatorTextPadding},
        {"tableAngledHeadersTextAlign", ImGuiStyleVar_TableAngledHeadersTextAlign},
    };

    ImGuiCol resolveColorKey(lua_State* L, int keyIdx, char const* method) {
        if (lua_isnumber(L, keyIdx)) {
            int col = static_cast<int>(lua_tointeger(L, keyIdx));
            if (col < 0 || col >= ImGuiCol_COUNT) {
                luaL_error(L, "%s: colors key out of range", method);
            }
            return static_cast<ImGuiCol>(col);
        }

        char const* name = checkLuaString(L, keyIdx);
        if (auto col = resolveImGuiColByName(name)) {
            return *col;
        }
        luaL_error(L, "%s: unknown color key '%s'", method, name);
    }

    void pushStyleVarsFromOpts(
        lua_State* L, int optsIdx, char const* method, ImGuiStyleVarPopGuard& guard
    ) {
        for (auto const& field : kFloatStyleFields) {
            lua_getfield(L, optsIdx, field.key);
            if (lua_isnumber(L, -1)) {
                float value = static_cast<float>(lua_tonumber(L, -1));
                ImGui::PushStyleVar(field.var, value);
                guard.push();
            }
            lua_pop(L, 1);
        }

        for (auto const& field : kVec2StyleFields) {
            ImVec2 vec;
            if (optFieldVec2(L, optsIdx, field.key, vec, method)) {
                ImGui::PushStyleVar(field.var, vec);
                guard.push();
            }
        }
    }

    void pushStyleColorsFromOpts(
        lua_State* L, int optsIdx, char const* method, ImGuiStyleColorPopGuard& guard
    ) {
        lua_getfield(L, optsIdx, "colors");
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return;
        }

        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            ImGuiCol col = resolveColorKey(L, -2, method);
            ImVec4 color = toImVec4(L, -1, method);
            ImGui::PushStyleColor(col, color);
            guard.push();
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    void applyFloatStyleField(lua_State* L, int optsIdx, char const* key, float& target) {
        lua_getfield(L, optsIdx, key);
        if (lua_isnumber(L, -1)) {
            target = static_cast<float>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1);
    }

    void applyVec2StyleField(
        lua_State* L, int optsIdx, char const* key, ImVec2& target, char const* method
    ) {
        ImVec2 vec;
        if (optFieldVec2(L, optsIdx, key, vec, method)) {
            target = vec;
        }
    }

    void applyStyleVarsGlobal(lua_State* L, int optsIdx, char const* method, ImGuiStyle& style) {
        applyFloatStyleField(L, optsIdx, "alpha", style.Alpha);
        applyFloatStyleField(L, optsIdx, "disabledAlpha", style.DisabledAlpha);
        applyFloatStyleField(L, optsIdx, "windowRounding", style.WindowRounding);
        applyFloatStyleField(L, optsIdx, "windowBorderSize", style.WindowBorderSize);
        applyFloatStyleField(L, optsIdx, "childRounding", style.ChildRounding);
        applyFloatStyleField(L, optsIdx, "childBorderSize", style.ChildBorderSize);
        applyFloatStyleField(L, optsIdx, "popupRounding", style.PopupRounding);
        applyFloatStyleField(L, optsIdx, "popupBorderSize", style.PopupBorderSize);
        applyFloatStyleField(L, optsIdx, "frameRounding", style.FrameRounding);
        applyFloatStyleField(L, optsIdx, "frameBorderSize", style.FrameBorderSize);
        applyFloatStyleField(L, optsIdx, "indentSpacing", style.IndentSpacing);
        applyFloatStyleField(L, optsIdx, "scrollbarSize", style.ScrollbarSize);
        applyFloatStyleField(L, optsIdx, "scrollbarRounding", style.ScrollbarRounding);
        applyFloatStyleField(L, optsIdx, "scrollbarPadding", style.ScrollbarPadding);
        applyFloatStyleField(L, optsIdx, "grabMinSize", style.GrabMinSize);
        applyFloatStyleField(L, optsIdx, "grabRounding", style.GrabRounding);
        applyFloatStyleField(L, optsIdx, "imageRounding", style.ImageRounding);
        applyFloatStyleField(L, optsIdx, "imageBorderSize", style.ImageBorderSize);
        applyFloatStyleField(L, optsIdx, "tabRounding", style.TabRounding);
        applyFloatStyleField(L, optsIdx, "tabBorderSize", style.TabBorderSize);
        applyFloatStyleField(L, optsIdx, "tabMinWidthBase", style.TabMinWidthBase);
        applyFloatStyleField(L, optsIdx, "tabMinWidthShrink", style.TabMinWidthShrink);
        applyFloatStyleField(L, optsIdx, "tabBarBorderSize", style.TabBarBorderSize);
        applyFloatStyleField(L, optsIdx, "tabBarOverlineSize", style.TabBarOverlineSize);
        applyFloatStyleField(L, optsIdx, "tableAngledHeadersAngle", style.TableAngledHeadersAngle);
        applyFloatStyleField(L, optsIdx, "treeLinesSize", style.TreeLinesSize);
        applyFloatStyleField(L, optsIdx, "treeLinesRounding", style.TreeLinesRounding);
        applyFloatStyleField(L, optsIdx, "dragDropTargetRounding", style.DragDropTargetRounding);
        applyFloatStyleField(L, optsIdx, "separatorSize", style.SeparatorSize);
        applyFloatStyleField(L, optsIdx, "separatorTextBorderSize", style.SeparatorTextBorderSize);

        applyVec2StyleField(L, optsIdx, "windowPadding", style.WindowPadding, method);
        applyVec2StyleField(L, optsIdx, "windowMinSize", style.WindowMinSize, method);
        applyVec2StyleField(L, optsIdx, "windowTitleAlign", style.WindowTitleAlign, method);
        applyVec2StyleField(L, optsIdx, "framePadding", style.FramePadding, method);
        applyVec2StyleField(L, optsIdx, "itemSpacing", style.ItemSpacing, method);
        applyVec2StyleField(L, optsIdx, "itemInnerSpacing", style.ItemInnerSpacing, method);
        applyVec2StyleField(L, optsIdx, "cellPadding", style.CellPadding, method);
        applyVec2StyleField(L, optsIdx, "buttonTextAlign", style.ButtonTextAlign, method);
        applyVec2StyleField(L, optsIdx, "selectableTextAlign", style.SelectableTextAlign, method);
        applyVec2StyleField(L, optsIdx, "separatorTextAlign", style.SeparatorTextAlign, method);
        applyVec2StyleField(L, optsIdx, "separatorTextPadding", style.SeparatorTextPadding, method);
        applyVec2StyleField(
            L, optsIdx, "tableAngledHeadersTextAlign", style.TableAngledHeadersTextAlign, method
        );
    }

    void applyStyleColorsGlobal(lua_State* L, int optsIdx, char const* method, ImGuiStyle& style) {
        lua_getfield(L, optsIdx, "colors");
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return;
        }

        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            ImGuiCol col = resolveColorKey(L, -2, method);
            style.Colors[col] = toImVec4(L, -1, method);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    int imguiStyleWith(lua_State* L) {
        requireFrame(L, "imgui.style.with");
        luaL_checktype(L, 1, LUA_TTABLE);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImGuiStyleVarPopGuard varGuard;
        ImGuiStyleColorPopGuard colorGuard;
        pushStyleVarsFromOpts(L, 1, "imgui.style.with", varGuard);
        pushStyleColorsFromOpts(L, 1, "imgui.style.with", colorGuard);
        callDrawClosure(L, 2, "imgui.style.with");
        return 0;
    }

    int imguiThemeApply(lua_State* L) {
        requireFrame(L, "imgui.theme.apply");
        char const* name = checkLuaString(L, 1);
        if (std::strcmp(name, "dark") == 0) {
            ImGui::StyleColorsDark();
        }
        else if (std::strcmp(name, "light") == 0) {
            ImGui::StyleColorsLight();
        }
        else if (std::strcmp(name, "classic") == 0) {
            ImGui::StyleColorsClassic();
        }
        else {
            luaL_error(L, "imgui.theme.apply: expected 'dark', 'light', or 'classic'");
        }
        return 0;
    }

    int imguiThemeApplyCustom(lua_State* L) {
        requireFrame(L, "imgui.theme.applyCustom");
        luaL_checktype(L, 1, LUA_TTABLE);
        ImGuiStyle& style = ImGui::GetStyle();
        applyStyleVarsGlobal(L, 1, "imgui.theme.applyCustom", style);
        applyStyleColorsGlobal(L, 1, "imgui.theme.applyCustom", style);
        return 0;
    }
} // namespace

namespace luax {
    void registerImGuiStyleAndTheme(lua_State* L) {
        ensureNestedTable(L, "style");
        setTableCFunction(L, -1, "with", &imguiStyleWith);
        lua_pop(L, 1);

        ensureNestedTable(L, "theme");
        setTableCFunction(L, -1, "apply", &imguiThemeApply);
        setTableCFunction(L, -1, "applyCustom", &imguiThemeApplyCustom);
        lua_pop(L, 1);
    }
} // namespace luax
