#pragma once

#include "core/Runtime.hpp"
#include "framework/usertype/DeferredRelease.hpp"

#include <cstdint>
#include <filesystem>
#include <lua.h>
#include <lualib.h>

namespace luax {
    class LuaRef {
    public:
        LuaRef() = default;

        LuaRef(lua_State* L, int index) {
            reset(L, index);
        }

        ~LuaRef() {
            reset();
        }

        LuaRef(LuaRef const&) = delete;
        LuaRef& operator=(LuaRef const&) = delete;

        LuaRef(LuaRef&& other) noexcept {
            moveFrom(other);
        }

        LuaRef& operator=(LuaRef&& other) noexcept {
            if (this != &other) {
                reset();
                moveFrom(other);
            }
            return *this;
        }

        void reset() {
            if (m_state && m_ref != LUA_NOREF && m_ref != LUA_REFNIL) {
                auto* runtime = Runtime::getIfInitialized();
                // Runtime-null and generation-mismatch drops are not leaks,
                // the owning state is gone or about to be closed.
                // Thank you for reading.
                if (runtime && m_generation == runtime->generation()) {
                    if (Runtime::isMainThread()) {
                        lua_unref(m_state, m_ref);
                    }
                    else {
                        deferLuaRefUnref(m_state, m_ref, m_generation);
                    }
                }
            }
            m_state = nullptr;
            m_ref = LUA_NOREF;
            m_generation = 0;
            m_resourcesRoot.clear();
        }

        void reset(lua_State* L, int index) {
            reset();
            if (!L) return;
            auto* runtime = Runtime::getIfInitialized();
            if (!runtime) return;
            m_state = lua_mainthread(L);
            m_generation = runtime->generation();
            m_resourcesRoot = runtime->resourcesRoot();
            lua_pushvalue(L, index);
            m_ref = lua_ref(L, -1);
            lua_pop(L, 1);
        }

        [[nodiscard]] bool valid() const {
            if (!m_state || m_ref == LUA_NOREF || m_ref == LUA_REFNIL) return false;
            auto* runtime = Runtime::getIfInitialized();
            return runtime && m_generation == runtime->generation();
        }

        [[nodiscard]] bool push() const {
            if (!valid()) return false;
            lua_getref(m_state, m_ref);
            return true;
        }

        lua_State* luaState() const noexcept {
            return m_state;
        }

        std::filesystem::path const& resourcesRoot() const {
            return m_resourcesRoot;
        }

    private:
        void moveFrom(LuaRef& other) noexcept {
            m_state = other.m_state;
            m_ref = other.m_ref;
            m_generation = other.m_generation;
            m_resourcesRoot = std::move(other.m_resourcesRoot);
            other.m_state = nullptr;
            other.m_ref = LUA_NOREF;
            other.m_generation = 0;
            other.m_resourcesRoot.clear();
        }

        lua_State* m_state = nullptr;
        int m_ref = LUA_NOREF;
        std::uint32_t m_generation = 0;
        std::filesystem::path m_resourcesRoot;
    };
} // namespace luax
