#include "bindings/geode/ModSandbox.hpp"
#include "bindings/imgui/ImGuiBindingInternal.hpp"
#include "bindings/imgui/ImGuiConstants.manifest.hpp"
#include "bindings/imgui/ImGuiFontRegistry.hpp"
#include "core/Runtime.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"
#include "framework/stack/UserdataTags.hpp"

#include <Geode/utils/ranges.hpp>
#include <cstring>
#include <imgui.h>
#include <lua.h>
#include <lualib.h>
#include <optional>

namespace {
    using namespace luax;

    enum class StyleTag : std::uint8_t {
        Float,
        Vec2
    };

    struct StyleField {
        char const* key;
        ImGuiStyleVar var;
        float ImGuiStyle::* floatMember;
        ImVec2 ImGuiStyle::* vec2Member;
        StyleTag tag;
    };

    constexpr StyleField kStyleFields[] = {
        {"alpha", ImGuiStyleVar_Alpha, &ImGuiStyle::Alpha, nullptr, StyleTag::Float},
        {"disabledAlpha",
         ImGuiStyleVar_DisabledAlpha,
         &ImGuiStyle::DisabledAlpha,
         nullptr,
         StyleTag::Float},
        {"windowRounding",
         ImGuiStyleVar_WindowRounding,
         &ImGuiStyle::WindowRounding,
         nullptr,
         StyleTag::Float},
        {"windowBorderSize",
         ImGuiStyleVar_WindowBorderSize,
         &ImGuiStyle::WindowBorderSize,
         nullptr,
         StyleTag::Float},
        {"childRounding",
         ImGuiStyleVar_ChildRounding,
         &ImGuiStyle::ChildRounding,
         nullptr,
         StyleTag::Float},
        {"childBorderSize",
         ImGuiStyleVar_ChildBorderSize,
         &ImGuiStyle::ChildBorderSize,
         nullptr,
         StyleTag::Float},
        {"popupRounding",
         ImGuiStyleVar_PopupRounding,
         &ImGuiStyle::PopupRounding,
         nullptr,
         StyleTag::Float},
        {"popupBorderSize",
         ImGuiStyleVar_PopupBorderSize,
         &ImGuiStyle::PopupBorderSize,
         nullptr,
         StyleTag::Float},
        {"frameRounding",
         ImGuiStyleVar_FrameRounding,
         &ImGuiStyle::FrameRounding,
         nullptr,
         StyleTag::Float},
        {"frameBorderSize",
         ImGuiStyleVar_FrameBorderSize,
         &ImGuiStyle::FrameBorderSize,
         nullptr,
         StyleTag::Float},
        {"indentSpacing",
         ImGuiStyleVar_IndentSpacing,
         &ImGuiStyle::IndentSpacing,
         nullptr,
         StyleTag::Float},
        {"scrollbarSize",
         ImGuiStyleVar_ScrollbarSize,
         &ImGuiStyle::ScrollbarSize,
         nullptr,
         StyleTag::Float},
        {"scrollbarRounding",
         ImGuiStyleVar_ScrollbarRounding,
         &ImGuiStyle::ScrollbarRounding,
         nullptr,
         StyleTag::Float},
        {"scrollbarPadding",
         ImGuiStyleVar_ScrollbarPadding,
         &ImGuiStyle::ScrollbarPadding,
         nullptr,
         StyleTag::Float},
        {"grabMinSize", ImGuiStyleVar_GrabMinSize, &ImGuiStyle::GrabMinSize, nullptr, StyleTag::Float},
        {"grabRounding", ImGuiStyleVar_GrabRounding, &ImGuiStyle::GrabRounding, nullptr, StyleTag::Float},
        {"imageRounding",
         ImGuiStyleVar_ImageRounding,
         &ImGuiStyle::ImageRounding,
         nullptr,
         StyleTag::Float},
        {"imageBorderSize",
         ImGuiStyleVar_ImageBorderSize,
         &ImGuiStyle::ImageBorderSize,
         nullptr,
         StyleTag::Float},
        {"tabRounding", ImGuiStyleVar_TabRounding, &ImGuiStyle::TabRounding, nullptr, StyleTag::Float},
        {"tabBorderSize",
         ImGuiStyleVar_TabBorderSize,
         &ImGuiStyle::TabBorderSize,
         nullptr,
         StyleTag::Float},
        {"tabMinWidthBase",
         ImGuiStyleVar_TabMinWidthBase,
         &ImGuiStyle::TabMinWidthBase,
         nullptr,
         StyleTag::Float},
        {"tabMinWidthShrink",
         ImGuiStyleVar_TabMinWidthShrink,
         &ImGuiStyle::TabMinWidthShrink,
         nullptr,
         StyleTag::Float},
        {"tabBarBorderSize",
         ImGuiStyleVar_TabBarBorderSize,
         &ImGuiStyle::TabBarBorderSize,
         nullptr,
         StyleTag::Float},
        {"tabBarOverlineSize",
         ImGuiStyleVar_TabBarOverlineSize,
         &ImGuiStyle::TabBarOverlineSize,
         nullptr,
         StyleTag::Float},
        {"tableAngledHeadersAngle",
         ImGuiStyleVar_TableAngledHeadersAngle,
         &ImGuiStyle::TableAngledHeadersAngle,
         nullptr,
         StyleTag::Float},
        {"treeLinesSize",
         ImGuiStyleVar_TreeLinesSize,
         &ImGuiStyle::TreeLinesSize,
         nullptr,
         StyleTag::Float},
        {"treeLinesRounding",
         ImGuiStyleVar_TreeLinesRounding,
         &ImGuiStyle::TreeLinesRounding,
         nullptr,
         StyleTag::Float},
        {"dragDropTargetRounding",
         ImGuiStyleVar_DragDropTargetRounding,
         &ImGuiStyle::DragDropTargetRounding,
         nullptr,
         StyleTag::Float},
        {"separatorSize",
         ImGuiStyleVar_SeparatorSize,
         &ImGuiStyle::SeparatorSize,
         nullptr,
         StyleTag::Float},
        {"separatorTextBorderSize",
         ImGuiStyleVar_SeparatorTextBorderSize,
         &ImGuiStyle::SeparatorTextBorderSize,
         nullptr,
         StyleTag::Float},
        {"windowPadding", ImGuiStyleVar_WindowPadding, nullptr, &ImGuiStyle::WindowPadding, StyleTag::Vec2},
        {"windowMinSize", ImGuiStyleVar_WindowMinSize, nullptr, &ImGuiStyle::WindowMinSize, StyleTag::Vec2},
        {"windowTitleAlign",
         ImGuiStyleVar_WindowTitleAlign,
         nullptr,
         &ImGuiStyle::WindowTitleAlign,
         StyleTag::Vec2},
        {"framePadding", ImGuiStyleVar_FramePadding, nullptr, &ImGuiStyle::FramePadding, StyleTag::Vec2},
        {"itemSpacing", ImGuiStyleVar_ItemSpacing, nullptr, &ImGuiStyle::ItemSpacing, StyleTag::Vec2},
        {"itemInnerSpacing",
         ImGuiStyleVar_ItemInnerSpacing,
         nullptr,
         &ImGuiStyle::ItemInnerSpacing,
         StyleTag::Vec2},
        {"cellPadding", ImGuiStyleVar_CellPadding, nullptr, &ImGuiStyle::CellPadding, StyleTag::Vec2},
        {"buttonTextAlign",
         ImGuiStyleVar_ButtonTextAlign,
         nullptr,
         &ImGuiStyle::ButtonTextAlign,
         StyleTag::Vec2},
        {"selectableTextAlign",
         ImGuiStyleVar_SelectableTextAlign,
         nullptr,
         &ImGuiStyle::SelectableTextAlign,
         StyleTag::Vec2},
        {"separatorTextAlign",
         ImGuiStyleVar_SeparatorTextAlign,
         nullptr,
         &ImGuiStyle::SeparatorTextAlign,
         StyleTag::Vec2},
        {"separatorTextPadding",
         ImGuiStyleVar_SeparatorTextPadding,
         nullptr,
         &ImGuiStyle::SeparatorTextPadding,
         StyleTag::Vec2},
        {"tableAngledHeadersTextAlign",
         ImGuiStyleVar_TableAngledHeadersTextAlign,
         nullptr,
         &ImGuiStyle::TableAngledHeadersTextAlign,
         StyleTag::Vec2},
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
        for (auto const& field : kStyleFields) {
            if (field.tag == StyleTag::Float) {
                lua_getfield(L, optsIdx, field.key);
                if (lua_isnumber(L, -1)) {
                    float value = static_cast<float>(lua_tonumber(L, -1));
                    ImGui::PushStyleVar(field.var, value);
                    guard.push();
                }
                lua_pop(L, 1);
            }
            else {
                ImVec2 vec;
                if (optFieldVec2(L, optsIdx, field.key, vec, method)) {
                    ImGui::PushStyleVar(field.var, vec);
                    guard.push();
                }
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
        for (auto const& field : kStyleFields) {
            if (field.tag == StyleTag::Float) {
                applyFloatStyleField(L, optsIdx, field.key, style.*(field.floatMember));
            }
            else {
                applyVec2StyleField(L, optsIdx, field.key, style.*(field.vec2Member), method);
            }
        }
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
        luaL_Reg const styleMethods[] = {
            {"with", &imguiStyleWith},
            {nullptr, nullptr},
        };
        applyLuaLReg(L, -1, styleMethods);
        lua_pop(L, 1);

        ensureNestedTable(L, "theme");
        luaL_Reg const themeMethods[] = {
            {"apply", &imguiThemeApply},
            {"applyCustom", &imguiThemeApplyCustom},
            {nullptr, nullptr},
        };
        applyLuaLReg(L, -1, themeMethods);
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

    bool hasSfntMagic(std::vector<std::uint8_t> const& data) {
        if (data.size() < 4) {
            return false;
        }
        auto const magic = (static_cast<std::uint32_t>(data[0]) << 24) |
            (static_cast<std::uint32_t>(data[1]) << 16) |
            (static_cast<std::uint32_t>(data[2]) << 8) | static_cast<std::uint32_t>(data[3]);
        return magic == 0x00010000u || magic == 0x4F54544Fu || magic == 0x74727565u ||
            magic == 0x74746366u;
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

        auto bytes = std::move(contents.unwrap());
        if (!hasSfntMagic(bytes)) {
            return pushNilErr(L, "font file could not be loaded");
        }

        auto const id = imguiFontAdd(size, std::move(bytes));
        imguiFontAfterRegistryChange();

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
        luaL_Reg const fontMethods[] = {
            {"add", &luaImGuiFontAdd},
            {"with", &luaImGuiFontWith},
            {nullptr, nullptr},
        };
        applyLuaLReg(L, -1, fontMethods);
        lua_pop(L, 1);
    }
} // namespace luax

#include "bindings/imgui/ImGuiHost.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <imgui.h>
#include <ranges>
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
        auto entry = std::ranges::find_if(s_entries, [id](FontEntry const& item) {
            return item.id == id;
        });
        return entry != s_entries.end() ? entry->font : nullptr;
    }

    void imguiFontRemove(std::uint64_t id) {
        geode::utils::ranges::remove(s_entries, [id](FontEntry const& entry) {
            return entry.id == id;
        });
    }

    void imguiFontRebuildAtlas() {
        if (ImGui::GetCurrentContext() == nullptr) {
            return;
        }

        float const density = geode::utils::getDisplayFactor();

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
