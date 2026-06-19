#include "bindings/geode/ModSandbox.hpp"
#include "bindings/imgui/ImGuiBindingInternal.hpp"
#include "bindings/imgui/ImGuiFontRegistry.hpp"
#include "core/Runtime.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/stack/TaggedMetatable.hpp"
#include "framework/stack/UserdataTags.hpp"

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
