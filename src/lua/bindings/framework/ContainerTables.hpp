#pragma once

#include "Stack.hpp"
#include "Types.hpp"
#include "Usertype.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

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

    template <class K>
    inline K checkMapKey(lua_State* L, int idx, char const* label) {
        return detail::checkPrimitiveVectorElement<K>(L, idx, label);
    }

    template <class V>
    inline V checkMapValue(lua_State* L, int idx, char const* label) {
        if constexpr (std::is_pointer_v<V>) {
            using Pointee = std::remove_pointer_t<V>;
            if (lua_isnil(L, idx)) {
                return nullptr;
            }
            return Usertype<Pointee>::check(L, idx, label);
        }
        return detail::checkPrimitiveVectorElement<V>(L, idx, label);
    }

    template <class K>
    inline void pushMapKey(lua_State* L, K const& key) {
        detail::pushPrimitiveVectorElement(L, key);
    }

    template <class V>
    inline void pushMapValue(lua_State* L, V const& value) {
        if constexpr (std::is_pointer_v<V>) {
            using Pointee = std::remove_pointer_t<V>;
            if (value == nullptr) {
                lua_pushnil(L);
            } else {
                Usertype<Pointee>::pushBorrowed(L, value);
            }
        } else {
            detail::pushPrimitiveVectorElement(L, value);
        }
    }

    namespace detail {
        template <class K, class V, class Map>
        Map checkAssociativeMap(lua_State* L, int idx, char const* label) {
            idx = lua_absindex(L, idx);
            luaL_checktype(L, idx, LUA_TTABLE);
            Map out;
            lua_pushnil(L);
            while (lua_next(L, idx) != 0) {
                K key = checkMapKey<K>(L, -2, label);
                V value = checkMapValue<V>(L, -1, label);
                out[std::move(key)] = std::move(value);
                lua_pop(L, 1);
            }
            return out;
        }

        template <class K, class V, class Map>
        void pushAssociativeMap(lua_State* L, Map const& map) {
            lua_createtable(L, 0, static_cast<int>(map.size()));
            int tableIndex = lua_gettop(L);
            for (auto const& entry : map) {
                pushMapKey(L, entry.first);
                pushMapValue(L, entry.second);
                lua_rawset(L, tableIndex);
            }
        }

        template <class T, class Set>
        Set checkSetFromTable(lua_State* L, int idx, char const* label) {
            idx = lua_absindex(L, idx);
            luaL_checktype(L, idx, LUA_TTABLE);
            auto len = static_cast<lua_Integer>(lua_objlen(L, idx));
            if (len < 0) {
                luaL_error(L, "%s: invalid set length", label);
            }
            Set out;
            for (lua_Integer i = 1; i <= len; ++i) {
                lua_rawgeti(L, idx, i);
                out.insert(checkPrimitiveVectorElement<T>(L, -1, label));
                lua_pop(L, 1);
            }
            return out;
        }

        template <class T, class Set>
        void pushSetAsTable(lua_State* L, Set const& set) {
            lua_createtable(L, static_cast<int>(set.size()), 0);
            int tableIndex = lua_gettop(L);
            int i = 1;
            for (auto const& elem : set) {
                pushPrimitiveVectorElement(L, elem);
                lua_rawseti(L, tableIndex, i++);
            }
        }
    }

    template <class K, class V>
    gd::map<K, V> checkMap(lua_State* L, int idx, char const* label) {
        return detail::checkAssociativeMap<K, V, gd::map<K, V>>(L, idx, label);
    }

    template <class K, class V>
    gd::unordered_map<K, V> checkUnorderedMap(lua_State* L, int idx, char const* label) {
        return detail::checkAssociativeMap<K, V, gd::unordered_map<K, V>>(L, idx, label);
    }

    template <class K, class V>
    void pushMap(lua_State* L, gd::map<K, V> const& map) {
        detail::pushAssociativeMap<K, V, gd::map<K, V>>(L, map);
    }

    template <class K, class V>
    void pushUnorderedMap(lua_State* L, gd::unordered_map<K, V> const& map) {
        detail::pushAssociativeMap<K, V, gd::unordered_map<K, V>>(L, map);
    }

    template <class K, class V>
    void pushMap(lua_State* L, gd::map<K, V>* map) {
        if (map == nullptr) {
            lua_pushnil(L);
            return;
        }
        pushMap(L, *map);
    }

    template <class K, class V>
    void pushMap(lua_State* L, gd::map<K, V> const* map) {
        pushMap(L, const_cast<gd::map<K, V>*>(map));
    }

    template <class K, class V>
    void pushUnorderedMap(lua_State* L, gd::unordered_map<K, V>* map) {
        if (map == nullptr) {
            lua_pushnil(L);
            return;
        }
        pushUnorderedMap(L, *map);
    }

    template <class K, class V>
    void pushUnorderedMap(lua_State* L, gd::unordered_map<K, V> const* map) {
        pushUnorderedMap(L, const_cast<gd::unordered_map<K, V>*>(map));
    }

    template <class T>
    gd::set<T> checkSet(lua_State* L, int idx, char const* label) {
        return detail::checkSetFromTable<T, gd::set<T>>(L, idx, label);
    }

    template <class T>
    gd::unordered_set<T> checkUnorderedSet(lua_State* L, int idx, char const* label) {
        return detail::checkSetFromTable<T, gd::unordered_set<T>>(L, idx, label);
    }

    template <class T>
    void pushSet(lua_State* L, gd::set<T> const& set) {
        detail::pushSetAsTable<T, gd::set<T>>(L, set);
    }

    template <class T>
    void pushUnorderedSet(lua_State* L, gd::unordered_set<T> const& set) {
        detail::pushSetAsTable<T, gd::unordered_set<T>>(L, set);
    }

    template <class T>
    void pushSet(lua_State* L, gd::set<T>* set) {
        if (set == nullptr) {
            lua_pushnil(L);
            return;
        }
        pushSet(L, *set);
    }

    template <class T>
    void pushSet(lua_State* L, gd::set<T> const* set) {
        pushSet(L, const_cast<gd::set<T>*>(set));
    }

    template <class T>
    void pushUnorderedSet(lua_State* L, gd::unordered_set<T>* set) {
        if (set == nullptr) {
            lua_pushnil(L);
            return;
        }
        pushUnorderedSet(L, *set);
    }

    template <class T>
    void pushUnorderedSet(lua_State* L, gd::unordered_set<T> const* set) {
        pushUnorderedSet(L, const_cast<gd::unordered_set<T>*>(set));
    }
}
