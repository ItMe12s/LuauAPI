#include "bindings/imgui/ImGuiBindingInternal.hpp"

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
