#pragma once

#include "Stack.hpp"
#include "Types.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>

#include <cstdint>
#include <string>
#include <type_traits>

namespace luax::detail {
    template <class T>
    inline T checkPrimitiveVectorElement(lua_State* L, int idx, char const* label) {
        if constexpr (std::is_same_v<T, bool>) {
            return check<bool>(L, idx, label);
        } else if constexpr (std::is_same_v<T, float>) {
            return check<float>(L, idx, label);
        } else if constexpr (std::is_same_v<T, double>) {
            return check<double>(L, idx, label);
        } else if constexpr (std::is_same_v<T, int>) {
            return check<int>(L, idx, label);
        } else if constexpr (std::is_same_v<T, unsigned>) {
            return check<unsigned>(L, idx, label);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return check<std::string>(L, idx, label);
        } else if constexpr (std::is_same_v<T, gd::string>) {
            auto storage = check<std::string>(L, idx, label);
            return gd::string(storage.c_str());
        } else if constexpr (std::is_enum_v<T>) {
            return static_cast<T>(static_cast<int>(check<int>(L, idx, label)));
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) > sizeof(int) || !std::is_signed_v<T>) {
                T value{};
                if (!tryIntegerString<T>(L, idx, &value)) {
                    if (!lua_isnumber(L, idx)) {
                        luaL_error(L, "%s expected number or integer string at arg %d", label, idx);
                    }
                    value = static_cast<T>(lua_tointeger(L, idx));
                }
                return value;
            }
            return static_cast<T>(check<int>(L, idx, label));
        } else {
            return check<T>(L, idx, label);
        }
    }

    template <class T>
    inline void pushPrimitiveVectorElement(lua_State* L, T const& value) {
        if constexpr (std::is_same_v<T, bool>) {
            push(L, value);
        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            push(L, value);
        } else if constexpr (std::is_same_v<T, int>) {
            lua_pushnumber(L, static_cast<double>(value));
        } else if constexpr (std::is_same_v<T, unsigned>) {
            push(L, value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            push(L, value);
        } else if constexpr (std::is_same_v<T, gd::string>) {
            push(L, std::string(value.c_str()));
        } else if constexpr (std::is_enum_v<T>) {
            lua_pushnumber(L, static_cast<double>(static_cast<int>(value)));
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) > sizeof(int) || !std::is_signed_v<T>) {
                pushIntegerString(L, value);
            } else {
                lua_pushnumber(L, static_cast<double>(value));
            }
        } else {
            push(L, value);
        }
    }
}

namespace luax {
    template <class T>
    gd::vector<T> checkPrimitiveVector(lua_State* L, int idx, char const* label) {
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);
        auto len = static_cast<lua_Integer>(lua_objlen(L, idx));
        if (len < 0) {
            luaL_error(L, "%s: invalid vector length", label);
        }
        gd::vector<T> out;
        out.reserve(static_cast<std::size_t>(len));
        for (lua_Integer i = 1; i <= len; ++i) {
            lua_rawgeti(L, idx, i);
            out.push_back(detail::checkPrimitiveVectorElement<T>(L, -1, label));
            lua_pop(L, 1);
        }
        return out;
    }

    template <class T>
    void pushPrimitiveVector(lua_State* L, gd::vector<T> const& vector) {
        lua_createtable(L, static_cast<int>(vector.size()), 0);
        int tableIndex = lua_gettop(L);
        int i = 1;
        for (auto const& elem : vector) {
            detail::pushPrimitiveVectorElement(L, elem);
            lua_rawseti(L, tableIndex, i++);
        }
    }

    template <class T>
    void pushPrimitiveVector(lua_State* L, gd::vector<T>* vector) {
        if (vector == nullptr) {
            lua_pushnil(L);
            return;
        }
        pushPrimitiveVector(L, *vector);
    }

    template <class T>
    void pushPrimitiveVector(lua_State* L, gd::vector<T> const* vector) {
        pushPrimitiveVector(L, const_cast<gd::vector<T>*>(vector));
    }
}
