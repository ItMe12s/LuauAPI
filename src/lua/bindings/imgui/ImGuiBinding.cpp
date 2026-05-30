#include "ImGuiDrawScheduler.hpp"
#include "ImGuiHost.hpp"
#include "ImVecConv.hpp"

#include "lua/Config.hpp"
#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/LuaRef.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/runtime/Runtime.hpp"

#include <imgui.h>
#include <lua.h>
#include <lualib.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {
    using namespace luax;

    constexpr char const* kHandleMeta          = "luax.ImGuiDrawHandle";
    constexpr std::size_t kInputTextDefaultCap = 16384;
    constexpr std::size_t kInputTextMaxCap     = 65536;

    struct ImGuiDrawHandle {
        std::uint64_t id;
    };

    void pushHandle(lua_State* L, std::uint64_t id) {
        auto* handle = static_cast<ImGuiDrawHandle*>(lua_newuserdata(L, sizeof(ImGuiDrawHandle)));
        handle->id = id;
        luaL_getmetatable(L, kHandleMeta);
        lua_setmetatable(L, -2);
    }

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

    void callDrawClosure(lua_State* L, int fnIdx, char const* context) {
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return;
        lua_pushvalue(L, fnIdx);
        runtime->protectedCall(0, 0, context, kImGuiScriptDeadlineMs);
    }

    int imguiOnDraw(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        if (ImGuiDrawScheduler::get().full()) {
            luaL_error(L, "imgui.onDraw: too many draw callbacks (limit %d)",
                       static_cast<int>(kMaxImGuiDrawCallbacks));
        }
        LuaRef ref;
        ref.reset(L, 1);
        std::uint64_t id = ImGuiDrawScheduler::get().add(std::move(ref));
        initImGuiHost();
        pushHandle(L, id);
        return 1;
    }

    int imguiCancel(lua_State* L) {
        auto* handle = static_cast<ImGuiDrawHandle*>(luaL_checkudata(L, 1, kHandleMeta));
        ImGuiDrawScheduler::get().cancel(handle->id);
        return 0;
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
        std::string title = check<std::string>(L, 1, "imgui.window");
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
        bool visible = ImGui::Begin(title.c_str(), closable ? &open : nullptr, flags);
        if (visible) {
            callDrawClosure(L, 2, "imgui.window");
        }
        ImGui::End();

        lua_pushboolean(L, open);
        return 1;
    }

    int imguiChild(lua_State* L) {
        requireFrame(L, "imgui.child");
        std::string id = check<std::string>(L, 1, "imgui.child");
        luaL_checktype(L, 2, LUA_TFUNCTION);

        ImVec2 size(0.0f, 0.0f);
        if (lua_istable(L, 3)) {
            optFieldVec2(L, 3, "size", size, "imgui.child");
        }

        bool visible = ImGui::BeginChild(id.c_str(), size);
        if (visible) {
            callDrawClosure(L, 2, "imgui.child");
        }
        ImGui::EndChild();
        return 0;
    }

    int imguiText(lua_State* L) {
        requireFrame(L, "imgui.text");
        std::string text = check<std::string>(L, 1, "imgui.text");
        ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
        return 0;
    }

    int imguiButton(lua_State* L) {
        requireFrame(L, "imgui.button");
        std::string label = check<std::string>(L, 1, "imgui.button");
        ImVec2 size(0.0f, 0.0f);
        if (lua_istable(L, 2)) {
            optFieldVec2(L, 2, "size", size, "imgui.button");
        }
        lua_pushboolean(L, ImGui::Button(label.c_str(), size));
        return 1;
    }

    int imguiCheckbox(lua_State* L) {
        requireFrame(L, "imgui.checkbox");
        std::string label = check<std::string>(L, 1, "imgui.checkbox");
        bool value = check<bool>(L, 2, "imgui.checkbox");
        ImGui::Checkbox(label.c_str(), &value);
        lua_pushboolean(L, value);
        return 1;
    }

    int imguiSliderFloat(lua_State* L) {
        requireFrame(L, "imgui.sliderFloat");
        std::string label = check<std::string>(L, 1, "imgui.sliderFloat");
        float value = check<float>(L, 2, "imgui.sliderFloat");
        float vmin = check<float>(L, 3, "imgui.sliderFloat");
        float vmax = check<float>(L, 4, "imgui.sliderFloat");
        char const* fmt = lua_isnoneornil(L, 5) ? "%.3f" : luaL_checkstring(L, 5);
        ImGui::SliderFloat(label.c_str(), &value, vmin, vmax, fmt);
        lua_pushnumber(L, value);
        return 1;
    }

    int imguiSliderInt(lua_State* L) {
        requireFrame(L, "imgui.sliderInt");
        std::string label = check<std::string>(L, 1, "imgui.sliderInt");
        int value = check<int>(L, 2, "imgui.sliderInt");
        int vmin = check<int>(L, 3, "imgui.sliderInt");
        int vmax = check<int>(L, 4, "imgui.sliderInt");
        ImGui::SliderInt(label.c_str(), &value, vmin, vmax);
        lua_pushinteger(L, value);
        return 1;
    }

    int imguiInputText(lua_State* L) {
        requireFrame(L, "imgui.inputText");
        std::string label = check<std::string>(L, 1, "imgui.inputText");
        std::string value = check<std::string>(L, 2, "imgui.inputText");

        std::size_t cap = kInputTextDefaultCap;
        if (!lua_isnoneornil(L, 3)) {
            int requested = check<int>(L, 3, "imgui.inputText");
            if (requested < 1) requested = 1;
            cap = std::min(static_cast<std::size_t>(requested), kInputTextMaxCap);
        }

        std::vector<char> buffer(cap + 1, '\0');
        std::size_t copy = std::min(value.size(), cap);
        std::copy_n(value.data(), copy, buffer.data());

        ImGui::InputText(label.c_str(), buffer.data(), buffer.size());
        lua_pushstring(L, buffer.data());
        return 1;
    }

    int imguiInputTextMultiline(lua_State* L) {
        requireFrame(L, "imgui.inputTextMultiline");
        std::string label = check<std::string>(L, 1, "imgui.inputTextMultiline");
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

        std::vector<char> buffer(cap + 1, '\0');
        std::size_t copy = std::min(value.size(), cap);
        std::copy_n(value.data(), copy, buffer.data());

        ImGui::InputTextMultiline(label.c_str(), buffer.data(), buffer.size(), size);
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

    void registerHandleMetatable(lua_State* L) {
        if (luaL_newmetatable(L, kHandleMeta)) {
            lua_pushcfunction(L, &imguiCancel, "cancel");
            lua_setfield(L, -2, "cancel");
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            lua_pushstring(L, "ImGuiDrawHandle");
            lua_setfield(L, -2, "__type");
        }
        lua_pop(L, 1);
    }

    geode::Result<void> registerImGui(lua_State* L) {
        registerHandleMetatable(L);

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
            runtime->registerShutdownHook([] {
                ImGuiDrawScheduler::get().clear();
            });
        }

        return geode::Ok();
    }
}

LUAX_BINDING(imgui_lib, registerImGui)
