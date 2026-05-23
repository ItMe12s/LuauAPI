#pragma once

#include <lua.h>
#include <lualib.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace luax {
    inline void push(lua_State* L, bool v) { lua_pushboolean(L, v); }
    inline void push(lua_State* L, int v) { lua_pushinteger(L, v); }
    inline void push(lua_State* L, unsigned v) { lua_pushinteger(L, static_cast<int>(v)); }
    inline void push(lua_State* L, long long v) { lua_pushnumber(L, static_cast<double>(v)); }
    inline void push(lua_State* L, float v) { lua_pushnumber(L, v); }
    inline void push(lua_State* L, double v) { lua_pushnumber(L, v); }
    inline void push(lua_State* L, char const* v) {
        if (!v) { lua_pushnil(L); return; }
        lua_pushstring(L, v);
    }
    inline void push(lua_State* L, std::string const& v) { lua_pushlstring(L, v.data(), v.size()); }
    inline void push(lua_State* L, std::string_view v) { lua_pushlstring(L, v.data(), v.size()); }
    inline void push(lua_State* L, std::nullptr_t) { lua_pushnil(L); }

    template <class... Ts, std::size_t... Is>
    inline int pushTupleImpl(lua_State* L, std::tuple<Ts...> const& t, std::index_sequence<Is...>) {
        (push(L, std::get<Is>(t)), ...);
        return sizeof...(Ts);
    }

    template <class... Ts>
    inline int pushTuple(lua_State* L, std::tuple<Ts...> const& t) {
        return pushTupleImpl(L, t, std::index_sequence_for<Ts...>{});
    }

    template <class T>
    T check(lua_State* L, int idx, char const* method);

    template <>
    inline float check<float>(lua_State* L, int idx, char const* method) {
        if (!lua_isnumber(L, idx)) luaL_error(L, "%s expected number at arg %d", method, idx);
        return static_cast<float>(lua_tonumber(L, idx));
    }

    template <>
    inline double check<double>(lua_State* L, int idx, char const* method) {
        if (!lua_isnumber(L, idx)) luaL_error(L, "%s expected number at arg %d", method, idx);
        return lua_tonumber(L, idx);
    }

    template <>
    inline int check<int>(lua_State* L, int idx, char const* method) {
        if (!lua_isnumber(L, idx)) luaL_error(L, "%s expected integer at arg %d", method, idx);
        return static_cast<int>(lua_tointeger(L, idx));
    }

    template <>
    inline unsigned check<unsigned>(lua_State* L, int idx, char const* method) {
        if (!lua_isnumber(L, idx)) luaL_error(L, "%s expected integer at arg %d", method, idx);
        return static_cast<unsigned>(lua_tointeger(L, idx));
    }

    template <>
    inline bool check<bool>(lua_State* L, int idx, char const* method) {
        if (!lua_isboolean(L, idx)) luaL_error(L, "%s expected boolean at arg %d", method, idx);
        return lua_toboolean(L, idx) != 0;
    }

    template <>
    inline std::string check<std::string>(lua_State* L, int idx, char const* method) {
        size_t len = 0;
        char const* s = lua_tolstring(L, idx, &len);
        if (!s) luaL_error(L, "%s expected string at arg %d", method, idx);
        return std::string(s, len);
    }

    template <>
    inline char const* check<char const*>(lua_State* L, int idx, char const* method) {
        char const* s = lua_tostring(L, idx);
        if (!s) luaL_error(L, "%s expected string at arg %d", method, idx);
        return s;
    }

    inline float fieldNumber(lua_State* L, int tableIdx, char const* key, char const* method) {
        lua_getfield(L, tableIdx, key);
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            luaL_error(L, "%s expected number field '%s'", method, key);
        }
        float v = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return v;
    }
}
