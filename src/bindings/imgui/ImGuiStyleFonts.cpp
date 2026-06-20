#include "bindings/geode/ModSandbox.hpp"
#include "bindings/imgui/ImGuiBindingInternal.hpp"
#include "bindings/imgui/ImGuiConstants.manifest.hpp"
#include "bindings/imgui/ImGuiFontRegistry.hpp"
#include "core/Runtime.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"
#include "framework/stack/UserdataTags.hpp"

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

#include <Geode/utils/string.hpp>
#include <filesystem>
#include <imgui.h>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;

    inline constexpr char const* kImGuiFontMeta = "luax.ImGuiFontHandle";
    inline constexpr char const* kImGuiFontTypeName = "ImGuiFontHandle";

    void requireNotFrame(lua_State* L, char const* method) {
        if (ImGuiDrawScheduler::get().inFrame()) {
            luaL_error(L, "%s must not run inside an imgui.onDraw callback", method);
        }
    }

    void requireMainThread(lua_State* L, char const* method) {
        if (!Runtime::isMainThread()) {
            luaL_error(L, "%s must run on the main thread", method);
        }
    }

    bool hasTtfExtension(std::filesystem::path const& path) {
        return geode::utils::string::endsWith(
            geode::utils::string::toLower(geode::utils::string::pathToString(path.filename())),
            ".ttf"
        );
    }

    void pushFontHandle(lua_State* L, std::uint64_t id) {
        auto* handle = static_cast<ImGuiFontHandle*>(lua_newuserdatataggedwithmetatable(
            L, sizeof(ImGuiFontHandle), detail::imguiFontHandleTag()
        ));
        handle->id = id;
    }

    ImGuiFontHandle* checkFontHandle(lua_State* L, int idx, char const* method) {
        auto* handle = static_cast<ImGuiFontHandle*>(luaL_checkudata(L, idx, kImGuiFontMeta));
        if (handle == nullptr || handle->id == 0) {
            luaL_error(L, "%s: font handle is invalid", method);
        }
        return handle;
    }

    struct ImGuiFontPopGuard {
        ~ImGuiFontPopGuard() {
            ImGui::PopFont();
        }
    };

    int luaImGuiFontAdd(lua_State* L) {
        requireMainThread(L, "imgui.font.add");
        requireNotFrame(L, "imgui.font.add");

        float const size = check<float>(L, 3, "imgui.font.add");
        if (size <= 0.f) {
            luaL_error(L, "imgui.font.add: size must be greater than 0");
        }

        auto target = resolveSandboxTarget(L, 1, 2, "imgui.font.add");
        if (!target) {
            return 2;
        }

        if (!hasTtfExtension(target->path)) {
            return pushNilErr(L, "font path must use a .ttf extension");
        }

        auto contents = readSandboxBinaryFile(target->path);
        if (contents.isErr()) {
            return pushNilErr(L, contents.unwrapErr());
        }

        auto const id = imguiFontAdd(size, std::move(contents.unwrap()));
        imguiFontAfterRegistryChange();
        if (ImGui::GetCurrentContext() != nullptr && imguiFontResolve(id) == nullptr) {
            imguiFontRemove(id);
            imguiFontAfterRegistryChange();
            return pushNilErr(L, "font file could not be loaded");
        }

        pushFontHandle(L, id);
        return 1;
    }

    int luaImGuiFontWith(lua_State* L) {
        requireFrame(L, "imgui.font.with");
        auto* handle = checkFontHandle(L, 1, "imgui.font.with");
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImFont* font = imguiFontResolve(handle->id);
        if (font == nullptr) {
            luaL_error(L, "imgui.font.with: font handle is invalid");
        }
        ImGui::PushFont(font);
        ImGuiFontPopGuard popGuard;
        callDrawClosure(L, 2, "imgui.font.with");
        return 0;
    }

    void registerFontHandleMetatable(lua_State* L) {
        luaL_Reg const methods[] = {
            {nullptr, nullptr},
        };
        registerTaggedMetatable(
            L, kImGuiFontMeta, detail::imguiFontHandleTag(), methods, std::nullopt, std::nullopt, kImGuiFontTypeName
        );
    }
} // namespace

namespace luax {
    void registerImGuiFont(lua_State* L) {
        registerFontHandleMetatable(L);

        ensureNestedTable(L, "font");
        setTableCFunction(L, -1, "add", &luaImGuiFontAdd);
        setTableCFunction(L, -1, "with", &luaImGuiFontWith);
        lua_pop(L, 1);
    }
} // namespace luax

#include "bindings/imgui/ImGuiHost.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <imgui.h>
#include <vector>

namespace luax {
    namespace {
        struct FontEntry {
            std::uint64_t id = 0;
            std::vector<std::uint8_t> data;
            float size = 0.f;
            ImFont* font = nullptr;
        };

        std::vector<FontEntry> s_entries;
        std::uint64_t s_nextId = 1;
    } // namespace

    std::uint64_t imguiFontAdd(float size, std::vector<std::uint8_t> data) {
        FontEntry entry;
        entry.id = s_nextId++;
        entry.data = std::move(data);
        entry.size = size;
        s_entries.push_back(std::move(entry));
        return s_entries.back().id;
    }

    ImFont* imguiFontResolve(std::uint64_t id) {
        if (id == 0) {
            return nullptr;
        }
        for (auto const& entry : s_entries) {
            if (entry.id == id) {
                return entry.font;
            }
        }
        return nullptr;
    }

    void imguiFontRemove(std::uint64_t id) {
        s_entries.erase(
            std::remove_if(
                s_entries.begin(),
                s_entries.end(),
                [id](FontEntry const& entry) {
                    return entry.id == id;
                }
            ),
            s_entries.end()
        );
    }

    void imguiFontRebuildAtlas() {
        if (ImGui::GetCurrentContext() == nullptr) {
            return;
        }

        float const density = geode::utils::getDisplayFactor();
        if (s_entries.empty() && density <= 1.f) {
            return;
        }

        ImFontConfig cfg;
        if (density > 1.f) {
            cfg.RasterizerDensity = density;
        }

        auto& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontDefault(!s_entries.empty() || density > 1.f ? &cfg : nullptr);

        for (auto& entry : s_entries) {
            if (entry.data.empty()) {
                entry.font = nullptr;
                continue;
            }

            ImFontConfig entryCfg = cfg;
            entryCfg.FontDataOwnedByAtlas = false;
            entry.font = io.Fonts->AddFontFromMemoryTTF(
                entry.data.data(), static_cast<int>(entry.data.size()), entry.size, &entryCfg
            );
        }
    }

    void imguiFontAfterRegistryChange() {
        if (imguiHostIsInitialized()) {
            imguiHostRequestReload();
            return;
        }
        imguiFontRebuildAtlas();
    }

    void imguiFontClear() {
        s_entries.clear();
        s_nextId = 1;
    }
} // namespace luax
