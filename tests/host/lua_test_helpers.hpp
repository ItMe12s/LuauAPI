#pragma once

#include "bindings/geode/CurrentMod.hpp"
#include "bindings/imgui/ImGuiDrawScheduler.hpp"
#include "bindings/imgui/ImGuiFontRegistry.hpp"
#include "bindings/task/TaskScheduler.hpp"
#include "bindings/websocket/WebSocketInternal.hpp"
#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "framework/callback/LuaTrampolineRegistry.hpp"
#include "framework/usertype/Fields.hpp"
#include "framework/usertype/LuaRef.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/utils/main_thread.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <lua.h>
#include <lualib.h>

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

namespace luauapi_test {
    struct LuaStateDeleter {
        void operator()(lua_State* L) const {
            if (L) {
                lua_close(L);
            }
        }
    };

    using LuaStatePtr = std::unique_ptr<lua_State, LuaStateDeleter>;

    inline LuaStatePtr makeLuaState(bool openLibs = false) {
        auto* L = luaL_newstate();
        REQUIRE(L != nullptr);
        if (openLibs) {
            luaL_openlibs(L);
        }
        return LuaStatePtr(L);
    }

    inline std::string compile(std::string_view source) {
        return luax::Runtime::compileSource(source);
    }

    struct BindingGuard {
        BindingGuard() {
            luax::resetBindingsForTests();
        }

        ~BindingGuard() {
            luax::resetBindingsForTests();
        }
    };

    struct RuntimeGuard {
        RuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~RuntimeGuard() {
            luax::Runtime::resetForTests();
        }
    };

    struct BindingRuntimeGuard {
        BindingRuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::resetBindingsForTests();
        }

        ~BindingRuntimeGuard() {
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };

    struct ModRuntimeGuard {
        ModRuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~ModRuntimeGuard() {
            luax::invalidateCurrentModCache();
            geode::Mod::resetForTests();
            luax::Runtime::resetForTests();
        }
    };

    struct BindingModRuntimeGuard {
        BindingModRuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            luax::resetBindingsForTests();
        }

        ~BindingModRuntimeGuard() {
            luax::invalidateCurrentModCache();
            geode::Mod::resetForTests();
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };

    struct TaskSchedulerRuntimeGuard {
        TaskSchedulerRuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~TaskSchedulerRuntimeGuard() {
            luax::TaskScheduler::get().clear();
            luax::Runtime::resetForTests();
        }
    };

    struct ImGuiRuntimeGuard {
        ImGuiRuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~ImGuiRuntimeGuard() {
            luax::ImGuiDrawScheduler::get().clear();
            luax::Runtime::resetForTests();
        }
    };

    struct FieldsRuntimeGuard {
        FieldsRuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
        }

        ~FieldsRuntimeGuard() {
            luax::Fields::clear();
            luax::Runtime::resetForTests();
        }
    };

    struct HandleGcRuntimeGuard {
        BindingRuntimeGuard binding;

        ~HandleGcRuntimeGuard() {
            luax::ImGuiDrawScheduler::get().clear();
            luax::TaskScheduler::get().clear();
        }
    };

    struct MiscCorrectnessRuntimeGuard {
        BindingRuntimeGuard binding;

        ~MiscCorrectnessRuntimeGuard() {
            luax::TaskScheduler::get().clear();
            luax::clearOrphanTrampolines();
        }
    };

    struct ImGuiBindingRuntimeGuard {
        BindingModRuntimeGuard binding;

        ~ImGuiBindingRuntimeGuard() {
            luax::ImGuiDrawScheduler::get().clear();
            luax::imguiFontClear();
        }
    };

    struct WebSocketRuntimeGuard {
        WebSocketRuntimeGuard() {
            luax::Runtime::setMainThreadId(std::this_thread::get_id());
            geode::test::bindMainThreadToCurrent();
            luax::resetBindingsForTests();
        }

        ~WebSocketRuntimeGuard() {
            luax::clearWsState();
            geode::test::clearMainThreadQueue();
            luax::Runtime::resetForTests();
            luax::resetBindingsForTests();
        }
    };

    inline void loadFunction(lua_State* L, std::string_view source, char const* chunk = "=test") {
        auto bytecode = compile(source);
        REQUIRE(luau_load(L, chunk, bytecode.data(), bytecode.size(), 0) == 0);
    }

    inline luax::LuaRef makeCallback(lua_State* L, std::string_view source) {
        loadFunction(L, source);
        luax::LuaRef ref(L, -1);
        lua_pop(L, 1);
        return ref;
    }

    struct ScopedTempDir {
        std::filesystem::path path;

        explicit ScopedTempDir(std::string_view prefix = "luauapi_test_") :
            path(
                std::filesystem::temp_directory_path() /
                (std::string(prefix) +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))
            ) {
            REQUIRE(std::filesystem::create_directories(path));
        }

        ScopedTempDir(ScopedTempDir const&) = delete;
        ScopedTempDir& operator=(ScopedTempDir const&) = delete;

        ~ScopedTempDir() {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }
    };

    inline void writeTestFile(std::filesystem::path const& path, std::string_view contents) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        REQUIRE(out.good());
        out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        REQUIRE(out.good());
    }

    inline void writeTestFile(std::filesystem::path const& path, std::span<std::uint8_t const> bytes) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        REQUIRE(out.good());
        out.write(reinterpret_cast<char const*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        REQUIRE(out.good());
    }

    inline bool runScriptPcall(lua_State* L, std::string_view source, char const* chunk = "=test") {
        auto bytecode = compile(source);
        if (luau_load(L, chunk, bytecode.data(), bytecode.size(), 0) != 0) {
            return false;
        }
        if (lua_pcall(L, 0, 1, 0) != 0) {
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    inline bool runScriptVoid(lua_State* L, std::string_view source, char const* chunk = "=test") {
        auto bytecode = compile(source);
        if (luau_load(L, chunk, bytecode.data(), bytecode.size(), 0) != 0) {
            return false;
        }
        if (lua_pcall(L, 0, 0, 0) != 0) {
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    inline bool runScriptReturnsBool(lua_State* L, std::string_view source, char const* chunk = "=test") {
        if (!runScriptPcall(L, source, chunk)) {
            return false;
        }
        if (!lua_isboolean(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        bool const value = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return value;
    }

    inline std::optional<std::string> runScriptReturnsString(
        lua_State* L, std::string_view source, char const* chunk = "=test"
    ) {
        if (!runScriptPcall(L, source, chunk)) {
            return std::nullopt;
        }
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        size_t len = 0;
        char const* text = lua_tolstring(L, -1, &len);
        std::string value(text ? text : "", len);
        lua_pop(L, 1);
        return value;
    }

    inline std::optional<double> runScriptReturnsNumber(
        lua_State* L, std::string_view source, char const* chunk = "=test"
    ) {
        if (!runScriptPcall(L, source, chunk)) {
            return std::nullopt;
        }
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        double const value = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return value;
    }

    inline void collectGarbage(lua_State* L) {
        lua_gc(L, LUA_GCSTOP, 0);
        lua_gc(L, LUA_GCCOLLECT, 0);
        lua_gc(L, LUA_GCRESTART, 0);
    }
} // namespace luauapi_test
