#pragma once

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
                lua_unref(m_state, m_ref);
            }
            m_state = nullptr;
            m_ref = LUA_NOREF;
        }

        void reset(lua_State* L, int index) {
            reset();
            if (!L) return;
            m_state = L;
            lua_pushvalue(L, index);
            m_ref = lua_ref(L, -1);
            lua_pop(L, 1);
        }

        bool valid() const {
            return m_state && m_ref != LUA_NOREF && m_ref != LUA_REFNIL;
        }

        bool push() const {
            if (!valid()) return false;
            lua_getref(m_state, m_ref);
            return true;
        }

    private:
        void moveFrom(LuaRef& other) noexcept {
            m_state = other.m_state;
            m_ref = other.m_ref;
            other.m_state = nullptr;
            other.m_ref = LUA_NOREF;
        }

        lua_State* m_state = nullptr;
        int m_ref = LUA_NOREF;
    };
}
