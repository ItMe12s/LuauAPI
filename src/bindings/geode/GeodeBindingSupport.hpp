#pragma once

#include "framework/stack/Stack.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <type_traits>
#include <utility>

namespace luax::geode_detail {
    inline double checkFinite(lua_State* L, int idx, char const* method) {
        double value = check<double>(L, idx, method);
        if (!std::isfinite(value)) {
            luaL_error(L, "%s expected a finite number at arg %d", method, idx);
        }
        return value;
    }

    inline int checkByte(lua_State* L, int idx, char const* method) {
        double value = checkFinite(L, idx, method);
        if (value < 0.0 || value > 255.0 || std::floor(value) != value) {
            luaL_error(L, "%s expected an integer from 0 to 255 at arg %d", method, idx);
        }
        return static_cast<int>(value);
    }

    inline int checkByteField(lua_State* L, int tableIdx, char const* key, char const* method) {
        tableIdx = lua_absindex(L, tableIdx);
        lua_getfield(L, tableIdx, key);
        int value = checkByte(L, -1, method);
        lua_pop(L, 1);
        return value;
    }

    inline std::array<unsigned char, 4> checkColorChannels(lua_State* L, int idx, char const* method) {
        luaL_checktype(L, idx, LUA_TTABLE);
        idx = lua_absindex(L, idx);
        return {
            static_cast<unsigned char>(checkByteField(L, idx, "r", method)),
            static_cast<unsigned char>(checkByteField(L, idx, "g", method)),
            static_cast<unsigned char>(checkByteField(L, idx, "b", method)),
            static_cast<unsigned char>(checkByteField(L, idx, "a", method)),
        };
    }

    inline std::size_t checkDenseArray(lua_State* L, int idx, char const* method) {
        luaL_checktype(L, idx, LUA_TTABLE);
        idx = lua_absindex(L, idx);
        auto len = static_cast<std::size_t>(lua_objlen(L, idx));
        if (len == 0) luaL_error(L, "%s expected a non-empty array", method);
        if (len > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            luaL_error(L, "%s array is too large", method);
        }

        for (std::size_t i = 1; i <= len; ++i) {
            lua_rawgeti(L, idx, static_cast<int>(i));
            bool missing = lua_isnil(L, -1);
            lua_pop(L, 1);
            if (missing) luaL_error(L, "%s expected a dense array", method);
        }

        lua_pushnil(L);
        while (lua_next(L, idx) != 0) {
            if (lua_isnumber(L, -2)) {
                double key = lua_tonumber(L, -2);
                if (!std::isfinite(key) || std::floor(key) != key || key < 1.0 || key > len) {
                    lua_pop(L, 2);
                    luaL_error(L, "%s expected a dense array", method);
                }
            }
            lua_pop(L, 1);
        }
        return len;
    }

    template <class T>
    void destroyOwned(lua_State*, void* raw) {
        std::destroy_at(static_cast<T*>(raw));
    }

    template <class T>
    T* pushOwned(lua_State* L, int tag, T value) {
        static_assert(std::is_nothrow_move_constructible_v<T>);
        auto* storage = static_cast<T*>(lua_newuserdatataggedwithmetatable(L, sizeof(T), tag));
        return std::construct_at(storage, std::move(value));
    }
} // namespace luax::geode_detail
