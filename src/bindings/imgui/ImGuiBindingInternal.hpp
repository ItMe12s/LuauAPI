#pragma once

#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "bindings/imgui/ImVecConv.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <imgui.h>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <vector>

namespace luax {
    inline constexpr std::size_t kImGuiInputTextDefaultCap = 16384;
    inline constexpr std::size_t kImGuiInputTextMaxCap = 65536;

    inline thread_local std::vector<char> s_imGuiInputTextBuffer;

    inline std::vector<char>& imGuiInputTextBuffer(std::size_t cap) {
        if (s_imGuiInputTextBuffer.size() < cap + 1) {
            s_imGuiInputTextBuffer.resize(cap + 1);
        }
        std::fill(s_imGuiInputTextBuffer.begin(), s_imGuiInputTextBuffer.end(), '\0');
        return s_imGuiInputTextBuffer;
    }

    inline void requireFrame(lua_State* L, char const* method) {
        if (!Runtime::isMainThread()) {
            luaL_error(L, "%s must run on the main thread", method);
        }
        if (!ImGuiDrawScheduler::get().inFrame()) {
            luaL_error(L, "%s must run inside an imgui.onDraw callback", method);
        }
    }

    inline float optFieldNumber(lua_State* L, int tableIdx, char const* key, float def) {
        lua_getfield(L, tableIdx, key);
        float v = lua_isnumber(L, -1) ? static_cast<float>(lua_tonumber(L, -1)) : def;
        lua_pop(L, 1);
        return v;
    }

    inline bool optFieldBool(lua_State* L, int tableIdx, char const* key, bool def) {
        lua_getfield(L, tableIdx, key);
        bool v = lua_isboolean(L, -1) ? (lua_toboolean(L, -1) != 0) : def;
        lua_pop(L, 1);
        return v;
    }

    inline bool optFieldVec2(lua_State* L, int tableIdx, char const* key, ImVec2& out, char const* method) {
        lua_getfield(L, tableIdx, key);
        bool present = lua_istable(L, -1);
        if (present) out = toImVec2(L, -1, method);
        lua_pop(L, 1);
        return present;
    }

    inline char const* checkLuaString(lua_State* L, int index, std::size_t* len = nullptr) {
        luaL_checktype(L, index, LUA_TSTRING);
        return lua_tolstring(L, index, len);
    }

    inline void callDrawClosure(lua_State* L, int fnIdx, char const* context) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return;
        lua_pushvalue(L, fnIdx);
        (void)runtime->protectedCall(0, 0, context, kImGuiScriptDeadlineMs);
    }

    struct ImGuiEndGuard {
        enum class Kind : std::uint8_t {
            Window,
            Child,
            TabBar,
            TabItem,
            Popup,
            MenuBar,
            Menu,
            Group,
            Table,
        };

        explicit ImGuiEndGuard(Kind kind) : kind_(kind) {}

        ImGuiEndGuard(ImGuiEndGuard const&) = delete;
        ImGuiEndGuard& operator=(ImGuiEndGuard const&) = delete;

        ~ImGuiEndGuard() {
            switch (kind_) {
                case Kind::Window: ImGui::End(); break;
                case Kind::Child: ImGui::EndChild(); break;
                case Kind::TabBar: ImGui::EndTabBar(); break;
                case Kind::TabItem: ImGui::EndTabItem(); break;
                case Kind::Popup: ImGui::EndPopup(); break;
                case Kind::MenuBar: ImGui::EndMenuBar(); break;
                case Kind::Menu: ImGui::EndMenu(); break;
                case Kind::Group: ImGui::EndGroup(); break;
                case Kind::Table: ImGui::EndTable(); break;
            }
        }

    private:
        Kind kind_;
    };

    struct ImGuiStyleColorPopGuard {
        explicit ImGuiStyleColorPopGuard(int count = 0) : count_(count) {}

        ImGuiStyleColorPopGuard(ImGuiStyleColorPopGuard const&) = delete;
        ImGuiStyleColorPopGuard& operator=(ImGuiStyleColorPopGuard const&) = delete;

        void push(int count = 1) {
            count_ += count;
        }

        ~ImGuiStyleColorPopGuard() {
            if (count_ > 0) ImGui::PopStyleColor(count_);
        }

    private:
        int count_;
    };

    struct ImGuiStyleVarPopGuard {
        explicit ImGuiStyleVarPopGuard(int count = 0) : count_(count) {}

        ImGuiStyleVarPopGuard(ImGuiStyleVarPopGuard const&) = delete;
        ImGuiStyleVarPopGuard& operator=(ImGuiStyleVarPopGuard const&) = delete;

        void push(int count = 1) {
            count_ += count;
        }

        ~ImGuiStyleVarPopGuard() {
            if (count_ > 0) ImGui::PopStyleVar(count_);
        }

    private:
        int count_;
    };

    struct ImGuiIntEnumEntry {
        char const* name;
        int value;
    };

    inline void ensureNestedTable(lua_State* L, char const* name) {
        lua_getfield(L, -1, name);
        if (lua_istable(L, -1)) return;
        lua_pop(L, 1);
        lua_createtable(L, 0, 8);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, name);
    }

    template <typename T, std::size_t N>
    inline void registerImGuiIntEnumTable(lua_State* L, T const (&entries)[N], char const* tableName) {
        lua_createtable(L, 0, static_cast<int>(N));
        for (auto const& entry : entries) {
            setIntField(L, entry.name, entry.value);
        }
        lua_setfield(L, -2, tableName);
    }
} // namespace luax
