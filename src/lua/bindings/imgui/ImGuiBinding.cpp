#include "lua/Config.hpp"
#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/imgui/ImGuiDrawHandleBinding.hpp"
#include "lua/bindings/imgui/ImGuiDrawScheduler.hpp"
#include "lua/bindings/imgui/ImGuiHost.hpp"
#include "lua/bindings/imgui/ImVecConv.hpp"
#include "lua/runtime/Runtime.hpp"

#include <algorithm>
#include <cstdint>
#include <imgui.h>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <vector>

namespace {
    using namespace luax;

    thread_local std::vector<char> s_inputTextBuffer;

    std::vector<char>& inputTextBuffer(std::size_t cap) {
        if (s_inputTextBuffer.size() < cap + 1) s_inputTextBuffer.resize(cap + 1);
        std::fill(s_inputTextBuffer.begin(), s_inputTextBuffer.end(), '\0');
        return s_inputTextBuffer;
    }

    constexpr std::size_t kInputTextDefaultCap = 16384;
    constexpr std::size_t kInputTextMaxCap = 65536;

    void requireFrame(lua_State* L, char const* method) {
        if (!Runtime::isMainThread()) {
            luaL_error(L, "%s must run on the main thread", method);
        }
        if (!ImGuiDrawScheduler::get().inFrame()) {
            luaL_error(L, "%s must run inside an imgui.onDraw callback", method);
        }
    }

    float optFieldNumber(lua_State* L, int tableIdx, char const* key, float def) {
        lua_getfield(L, tableIdx, key);
        float v = lua_isnumber(L, -1) ? static_cast<float>(lua_tonumber(L, -1)) : def;
        lua_pop(L, 1);
        return v;
    }

    bool optFieldBool(lua_State* L, int tableIdx, char const* key, bool def) {
        lua_getfield(L, tableIdx, key);
        bool v = lua_isboolean(L, -1) ? (lua_toboolean(L, -1) != 0) : def;
        lua_pop(L, 1);
        return v;
    }

    bool optFieldVec2(lua_State* L, int tableIdx, char const* key, ImVec2& out, char const* method) {
        lua_getfield(L, tableIdx, key);
        bool present = lua_istable(L, -1);
        if (present) out = toImVec2(L, -1, method);
        lua_pop(L, 1);
        return present;
    }

    char const* checkLuaString(lua_State* L, int index, std::size_t* len = nullptr) {
        luaL_checktype(L, index, LUA_TSTRING);
        return lua_tolstring(L, index, len);
    }

    void callDrawClosure(lua_State* L, int fnIdx, char const* context) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return;
        lua_pushvalue(L, fnIdx);
        (void)runtime->protectedCall(0, 0, context, kImGuiScriptDeadlineMs);
    }

    int imguiSetVisible(lua_State* L) {
        imguiHostSetVisible(check<bool>(L, 1, "imgui.setVisible"));
        return 0;
    }

    int imguiToggle(lua_State*) {
        imguiHostToggle();
        return 0;
    }

    int imguiIsVisible(lua_State* L) {
        lua_pushboolean(L, imguiHostIsVisible());
        return 1;
    }

    int imguiWindow(lua_State* L) {
        requireFrame(L, "imgui.window");
        char const* title = checkLuaString(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImGuiWindowFlags flags = 0;
        bool closable = false;
        if (lua_istable(L, 3)) {
            ImVec2 vec;
            if (optFieldVec2(L, 3, "size", vec, "imgui.window")) {
                ImGui::SetNextWindowSize(vec, ImGuiCond_FirstUseEver);
            }
            if (optFieldVec2(L, 3, "pos", vec, "imgui.window")) {
                ImGui::SetNextWindowPos(vec, ImGuiCond_FirstUseEver);
            }
            flags = static_cast<ImGuiWindowFlags>(optFieldNumber(L, 3, "flags", 0.0f));
            closable = optFieldBool(L, 3, "closable", false);
        }

        bool open = true;
        bool visible = ImGui::Begin(title, closable ? &open : nullptr, flags);
        if (visible) callDrawClosure(L, 2, "imgui.window");
        ImGui::End();

        lua_pushboolean(L, open);
        return 1;
    }

    int imguiChild(lua_State* L) {
        requireFrame(L, "imgui.child");
        char const* id = checkLuaString(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImVec2 size(0.0f, 0.0f);
        if (lua_istable(L, 3)) {
            optFieldVec2(L, 3, "size", size, "imgui.child");
        }

        bool visible = ImGui::BeginChild(id, size);
        if (visible) callDrawClosure(L, 2, "imgui.child");
        ImGui::EndChild();
        return 0;
    }

    int imguiText(lua_State* L) {
        requireFrame(L, "imgui.text");
        size_t textLen = 0;
        char const* text = checkLuaString(L, 1, &textLen);
        ImGui::TextUnformatted(text, text + textLen);
        return 0;
    }

    int imguiButton(lua_State* L) {
        requireFrame(L, "imgui.button");
        char const* label = checkLuaString(L, 1);
        ImVec2 size(0.0f, 0.0f);
        if (lua_istable(L, 2)) {
            optFieldVec2(L, 2, "size", size, "imgui.button");
        }
        lua_pushboolean(L, ImGui::Button(label, size));
        return 1;
    }

    int imguiCheckbox(lua_State* L) {
        requireFrame(L, "imgui.checkbox");
        char const* label = checkLuaString(L, 1);
        bool value = check<bool>(L, 2, "imgui.checkbox");
        ImGui::Checkbox(label, &value);
        lua_pushboolean(L, value);
        return 1;
    }

    int imguiSliderFloat(lua_State* L) {
        requireFrame(L, "imgui.sliderFloat");
        char const* label = checkLuaString(L, 1);
        float value = check<float>(L, 2, "imgui.sliderFloat");
        float vmin = check<float>(L, 3, "imgui.sliderFloat");
        float vmax = check<float>(L, 4, "imgui.sliderFloat");
        char const* fmt = lua_isnoneornil(L, 5) ? "%.3f" : checkLuaString(L, 5);
        ImGui::SliderFloat(label, &value, vmin, vmax, fmt);
        lua_pushnumber(L, value);
        return 1;
    }

    int imguiSliderInt(lua_State* L) {
        requireFrame(L, "imgui.sliderInt");
        char const* label = checkLuaString(L, 1);
        int value = check<int>(L, 2, "imgui.sliderInt");
        int vmin = check<int>(L, 3, "imgui.sliderInt");
        int vmax = check<int>(L, 4, "imgui.sliderInt");
        ImGui::SliderInt(label, &value, vmin, vmax);
        lua_pushinteger(L, value);
        return 1;
    }

    int imguiInputText(lua_State* L) {
        requireFrame(L, "imgui.inputText");
        char const* label = checkLuaString(L, 1);
        std::string value = check<std::string>(L, 2, "imgui.inputText");

        std::size_t cap = kInputTextDefaultCap;
        if (!lua_isnoneornil(L, 3)) {
            int requested = check<int>(L, 3, "imgui.inputText");
            if (requested < 1) requested = 1;
            cap = std::min(static_cast<std::size_t>(requested), kInputTextMaxCap);
        }

        std::vector<char>& buffer = inputTextBuffer(cap);
        std::size_t copy = std::min(value.size(), cap);
        std::copy_n(value.data(), copy, buffer.data());

        ImGui::InputText(label, buffer.data(), buffer.size());
        lua_pushstring(L, buffer.data());
        return 1;
    }

    int imguiInputTextMultiline(lua_State* L) {
        requireFrame(L, "imgui.inputTextMultiline");
        char const* label = checkLuaString(L, 1);
        std::string value = check<std::string>(L, 2, "imgui.inputTextMultiline");

        ImVec2 size(0.0f, 0.0f);
        if (lua_istable(L, 3)) {
            size = toImVec2(L, 3, "imgui.inputTextMultiline");
        }

        std::size_t cap = kInputTextDefaultCap;
        if (!lua_isnoneornil(L, 4)) {
            int requested = check<int>(L, 4, "imgui.inputTextMultiline");
            if (requested < 1) requested = 1;
            cap = std::min(static_cast<std::size_t>(requested), kInputTextMaxCap);
        }

        std::vector<char>& buffer = inputTextBuffer(cap);
        std::size_t copy = std::min(value.size(), cap);
        std::copy_n(value.data(), copy, buffer.data());

        ImGui::InputTextMultiline(label, buffer.data(), buffer.size(), size);
        lua_pushstring(L, buffer.data());
        return 1;
    }

    int imguiSameLine(lua_State* L) {
        requireFrame(L, "imgui.sameLine");
        float offset = lua_isnoneornil(L, 1) ? 0.0f : check<float>(L, 1, "imgui.sameLine");
        float spacing = lua_isnoneornil(L, 2) ? -1.0f : check<float>(L, 2, "imgui.sameLine");
        ImGui::SameLine(offset, spacing);
        return 0;
    }

    int imguiSeparator(lua_State* L) {
        requireFrame(L, "imgui.separator");
        ImGui::Separator();
        return 0;
    }

    int imguiSpacing(lua_State* L) {
        requireFrame(L, "imgui.spacing");
        ImGui::Spacing();
        return 0;
    }

    int imguiGetContentRegionAvail(lua_State* L) {
        requireFrame(L, "imgui.getContentRegionAvail");
        pushImVec2(L, ImGui::GetContentRegionAvail());
        return 1;
    }

    geode::Result<void> registerImGui(lua_State* L) {
        registerImGuiDrawHandleMetatable(L);

        lua_newtable(L);
        setTableCFunction(L, -1, "onDraw", &imguiOnDraw);
        setTableCFunction(L, -1, "cancel", &imguiCancel);
        setTableCFunction(L, -1, "setVisible", &imguiSetVisible);
        setTableCFunction(L, -1, "toggle", &imguiToggle);
        setTableCFunction(L, -1, "isVisible", &imguiIsVisible);
        setTableCFunction(L, -1, "window", &imguiWindow);
        setTableCFunction(L, -1, "child", &imguiChild);
        setTableCFunction(L, -1, "text", &imguiText);
        setTableCFunction(L, -1, "button", &imguiButton);
        setTableCFunction(L, -1, "checkbox", &imguiCheckbox);
        setTableCFunction(L, -1, "sliderFloat", &imguiSliderFloat);
        setTableCFunction(L, -1, "sliderInt", &imguiSliderInt);
        setTableCFunction(L, -1, "inputText", &imguiInputText);
        setTableCFunction(L, -1, "inputTextMultiline", &imguiInputTextMultiline);
        setTableCFunction(L, -1, "sameLine", &imguiSameLine);
        setTableCFunction(L, -1, "separator", &imguiSeparator);
        setTableCFunction(L, -1, "spacing", &imguiSpacing);
        setTableCFunction(L, -1, "getContentRegionAvail", &imguiGetContentRegionAvail);
        lua_setglobal(L, "imgui");

        if (auto* runtime = static_cast<Runtime*>(lua_callbacks(L)->userdata)) {
            runtime->registerShutdownHook(&shutdownImGuiHost);
        }

        return geode::Ok();
    }
} // namespace

LUAX_BINDING(imgui_lib, registerImGui)
