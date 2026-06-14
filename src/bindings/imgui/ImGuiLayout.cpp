#include "bindings/imgui/ImGuiBindingInternal.hpp"

#include <imgui.h>
#include <lua.h>
#include <lualib.h>

namespace {
    using namespace luax;

    int imguiSeparatorText(lua_State* L) {
        requireFrame(L, "imgui.separatorText");
        ImGui::SeparatorText(checkLuaString(L, 1));
        return 0;
    }

    int imguiIndent(lua_State* L) {
        requireFrame(L, "imgui.indent");
        float width = lua_isnoneornil(L, 1) ? 0.0f : check<float>(L, 1, "imgui.indent");
        ImGui::Indent(width);
        return 0;
    }

    int imguiUnindent(lua_State* L) {
        requireFrame(L, "imgui.unindent");
        float width = lua_isnoneornil(L, 1) ? 0.0f : check<float>(L, 1, "imgui.unindent");
        ImGui::Unindent(width);
        return 0;
    }

    int imguiDummy(lua_State* L) {
        requireFrame(L, "imgui.dummy");
        ImVec2 size = toImVec2(L, 1, "imgui.dummy");
        ImGui::Dummy(size);
        return 0;
    }

    int imguiNewLine(lua_State* L) {
        requireFrame(L, "imgui.newLine");
        ImGui::NewLine();
        return 0;
    }

    int imguiCollapsingHeader(lua_State* L) {
        requireFrame(L, "imgui.collapsingHeader");
        char const* label = checkLuaString(L, 1);

        ImGuiTreeNodeFlags flags = 0;
        bool trackVisible = false;
        bool visible = true;
        if (lua_istable(L, 2)) {
            flags = static_cast<ImGuiTreeNodeFlags>(optFieldNumber(L, 2, "flags", 0.0f));
            lua_getfield(L, 2, "visible");
            if (lua_isboolean(L, -1)) {
                trackVisible = true;
                visible = lua_toboolean(L, -1) != 0;
            }
            lua_pop(L, 1);
        }

        bool expanded = trackVisible ? ImGui::CollapsingHeader(label, &visible, flags) :
                                       ImGui::CollapsingHeader(label, flags);
        lua_pushboolean(L, expanded);
        if (trackVisible) {
            lua_pushboolean(L, visible);
            return 2;
        }
        return 1;
    }

    int imguiTreeNode(lua_State* L) {
        requireFrame(L, "imgui.treeNode");
        char const* label = checkLuaString(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImGuiTreeNodeFlags flags = 0;
        bool trackOpen = false;
        bool open = false;
        if (lua_istable(L, 3)) {
            flags = static_cast<ImGuiTreeNodeFlags>(optFieldNumber(L, 3, "flags", 0.0f));
            lua_getfield(L, 3, "open");
            if (lua_isboolean(L, -1)) {
                trackOpen = true;
                open = lua_toboolean(L, -1) != 0;
                ImGui::SetNextItemOpen(open, ImGuiCond_Always);
            }
            lua_pop(L, 1);
        }

        bool expanded = ImGui::TreeNodeEx(label, flags);
        ImGuiTreePopGuard popGuard{expanded};
        if (expanded) callDrawClosure(L, 2, "imgui.treeNode");

        if (trackOpen) {
            open = expanded;
            lua_pushboolean(L, open);
            return 1;
        }
        return 0;
    }
} // namespace

namespace luax {
    void registerImGuiLayout(lua_State* L) {
        setTableCFunction(L, -1, "separatorText", &imguiSeparatorText);
        setTableCFunction(L, -1, "indent", &imguiIndent);
        setTableCFunction(L, -1, "unindent", &imguiUnindent);
        setTableCFunction(L, -1, "dummy", &imguiDummy);
        setTableCFunction(L, -1, "newLine", &imguiNewLine);
        setTableCFunction(L, -1, "collapsingHeader", &imguiCollapsingHeader);
        setTableCFunction(L, -1, "treeNode", &imguiTreeNode);
    }
} // namespace luax
