#include "bindings/imgui/ImGuiBindingInternal.hpp"
#include "bindings/imgui/ImGuiConstants.manifest.hpp"

#include <cstring>
#include <imgui.h>
#include <optional>

namespace luax {
    namespace {
        constexpr ImGuiIntEnumEntry kWindowFlagEntries[] = {
            LUAX_IMGUI_WINDOW_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kChildFlagEntries[] = {
            LUAX_IMGUI_CHILD_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kInputTextFlagEntries[] = {
            LUAX_IMGUI_INPUT_TEXT_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kTreeNodeFlagEntries[] = {
            LUAX_IMGUI_TREE_NODE_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kPopupFlagEntries[] = {
            LUAX_IMGUI_POPUP_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kSelectableFlagEntries[] = {
            LUAX_IMGUI_SELECTABLE_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kColorEditFlagEntries[] = {
            LUAX_IMGUI_COLOR_EDIT_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kSliderFlagEntries[] = {
            LUAX_IMGUI_SLIDER_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kComboFlagEntries[] = {
            LUAX_IMGUI_COMBO_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kTabBarFlagEntries[] = {
            LUAX_IMGUI_TAB_BAR_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kTabItemFlagEntries[] = {
            LUAX_IMGUI_TAB_ITEM_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kTableFlagEntries[] = {
            LUAX_IMGUI_TABLE_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kTableColumnFlagEntries[] = {
            LUAX_IMGUI_TABLE_COLUMN_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kHoveredFlagEntries[] = {
            LUAX_IMGUI_HOVERED_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kFocusedFlagEntries[] = {
            LUAX_IMGUI_FOCUSED_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kItemFlagEntries[] = {LUAX_IMGUI_ITEM_FLAGS(LUAX_IMGUI_ENUM_ENTRY)};
        constexpr ImGuiIntEnumEntry kDragDropFlagEntries[] = {
            LUAX_IMGUI_DRAG_DROP_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kMultiSelectFlagEntries[] = {
            LUAX_IMGUI_MULTI_SELECT_FLAGS(LUAX_IMGUI_ENUM_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kColEntries[] = {LUAX_IMGUI_COLORS(LUAX_IMGUI_COL_ENTRY)};
        constexpr ImGuiIntEnumEntry kStyleVarEntries[] = {
            LUAX_IMGUI_STYLE_VARS(LUAX_IMGUI_STYLE_VAR_ENTRY)
        };
        constexpr ImGuiIntEnumEntry kCondEntries[] = {LUAX_IMGUI_COND(LUAX_IMGUI_ENUM_ENTRY)};
    } // namespace

    std::optional<ImGuiCol> resolveImGuiColByName(char const* name) {
        for (auto const& entry : kColEntries) {
            if (std::strcmp(entry.name, name) == 0) {
                return static_cast<ImGuiCol>(entry.value);
            }
        }
        return std::nullopt;
    }

    void registerImGuiConstants(lua_State* L) {
        ensureNestedTable(L, "Flag");
        registerImGuiIntEnumSubgroup(L, kWindowFlagEntries, "Window");
        registerImGuiIntEnumSubgroup(L, kChildFlagEntries, "Child");
        registerImGuiIntEnumSubgroup(L, kInputTextFlagEntries, "InputText");
        registerImGuiIntEnumSubgroup(L, kTreeNodeFlagEntries, "TreeNode");
        registerImGuiIntEnumSubgroup(L, kPopupFlagEntries, "Popup");
        registerImGuiIntEnumSubgroup(L, kSelectableFlagEntries, "Selectable");
        registerImGuiIntEnumSubgroup(L, kColorEditFlagEntries, "ColorEdit");
        registerImGuiIntEnumSubgroup(L, kSliderFlagEntries, "Slider");
        registerImGuiIntEnumSubgroup(L, kComboFlagEntries, "Combo");
        registerImGuiIntEnumSubgroup(L, kTabBarFlagEntries, "TabBar");
        registerImGuiIntEnumSubgroup(L, kTabItemFlagEntries, "TabItem");
        registerImGuiIntEnumSubgroup(L, kTableFlagEntries, "Table");
        registerImGuiIntEnumSubgroup(L, kTableColumnFlagEntries, "TableColumn");
        registerImGuiIntEnumSubgroup(L, kHoveredFlagEntries, "Hovered");
        registerImGuiIntEnumSubgroup(L, kFocusedFlagEntries, "Focused");
        registerImGuiIntEnumSubgroup(L, kItemFlagEntries, "Item");
        registerImGuiIntEnumSubgroup(L, kDragDropFlagEntries, "DragDrop");
        registerImGuiIntEnumSubgroup(L, kMultiSelectFlagEntries, "MultiSelect");
        lua_pop(L, 1);

        registerImGuiIntEnumTable(L, kColEntries, "Col");
        registerImGuiIntEnumTable(L, kStyleVarEntries, "StyleVar");
        registerImGuiIntEnumTable(L, kCondEntries, "Cond");
    }
} // namespace luax
