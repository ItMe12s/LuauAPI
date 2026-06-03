#pragma once

#include "Stack.hpp"
#include "Types.hpp"
#include "Usertype.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <lualib.h>

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>

namespace luax {
    template <class F, class S>
    std::pair<F, S> checkPair(lua_State* L, int idx, char const* label);

    template <class F, class S>
    void pushPair(lua_State* L, std::pair<F, S> const& pair);
}

namespace luax::detail {
    template <class T>
    struct is_std_pair : std::false_type {};

    template <class F, class S>
    struct is_std_pair<std::pair<F, S>> : std::true_type {};

    template <class T>
    inline constexpr bool is_std_pair_v = is_std_pair<T>::value;

    inline void requireTableField(lua_State* L, int idx, char const* name, char const* label) {
        lua_getfield(L, idx, name);
        if (lua_isnil(L, -1)) {
            luaL_error(L, "%s missing field %s", label, name);
        }
    }

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
        } else if constexpr (is_std_pair_v<T>) {
            return luax::checkPair<typename T::first_type, typename T::second_type>(L, idx, label);
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
        } else if constexpr (is_std_pair_v<T>) {
            luax::pushPair<typename T::first_type, typename T::second_type>(L, value);
        } else {
            push(L, value);
        }
    }
}

namespace luax {
    template <class F, class S>
    std::pair<F, S> checkPair(lua_State* L, int idx, char const* label) {
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);
        detail::requireTableField(L, idx, "first", label);
        F first = detail::checkPrimitiveVectorElement<F>(L, -1, label);
        lua_pop(L, 1);
        detail::requireTableField(L, idx, "second", label);
        S second = detail::checkPrimitiveVectorElement<S>(L, -1, label);
        lua_pop(L, 1);
        return std::make_pair(std::move(first), std::move(second));
    }

    template <class F, class S>
    void pushPair(lua_State* L, std::pair<F, S> const& pair) {
        lua_createtable(L, 0, 2);
        int tableIndex = lua_gettop(L);
        detail::pushPrimitiveVectorElement(L, pair.first);
        lua_setfield(L, tableIndex, "first");
        detail::pushPrimitiveVectorElement(L, pair.second);
        lua_setfield(L, tableIndex, "second");
    }

    template <class F, class S>
    void pushPair(lua_State* L, std::pair<F, S>* pair) {
        if (pair == nullptr) {
            lua_pushnil(L);
            return;
        }
        pushPair(L, *pair);
    }

    template <class F, class S>
    void pushPair(lua_State* L, std::pair<F, S> const* pair) {
        pushPair(L, const_cast<std::pair<F, S>*>(pair));
    }
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

    template <class T, std::size_t N>
    std::array<T, N> checkStdArray(lua_State* L, int idx, char const* label) {
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);
        auto len = static_cast<lua_Integer>(lua_objlen(L, idx));
        if (len != static_cast<lua_Integer>(N)) {
            luaL_error(
                L,
                "%s: expected array length %zu, got %lld",
                label,
                N,
                static_cast<long long>(len)
            );
        }
        std::array<T, N> out{};
        for (std::size_t i = 0; i < N; ++i) {
            lua_rawgeti(L, idx, static_cast<int>(i + 1));
            out[i] = detail::checkPrimitiveVectorElement<T>(L, -1, label);
            lua_pop(L, 1);
        }
        return out;
    }

    template <class T, std::size_t N>
    void pushStdArray(lua_State* L, std::array<T, N> const& array) {
        lua_createtable(L, static_cast<int>(N), 0);
        int tableIndex = lua_gettop(L);
        int i = 1;
        for (auto const& elem : array) {
            detail::pushPrimitiveVectorElement(L, elem);
            lua_rawseti(L, tableIndex, i++);
        }
    }

    template <class T, std::size_t N>
    void pushStdArray(lua_State* L, std::array<T, N>* array) {
        if (array == nullptr) {
            lua_pushnil(L);
            return;
        }
        pushStdArray<T, N>(L, *array);
    }

    template <class T, std::size_t N>
    void pushStdArray(lua_State* L, std::array<T, N> const* array) {
        pushStdArray<T, N>(L, const_cast<std::array<T, N>*>(array));
    }

    template <class T, std::size_t N, class U>
    void pushStdArray(lua_State* L, U const (&array)[N]) {
        lua_createtable(L, static_cast<int>(N), 0);
        int tableIndex = lua_gettop(L);
        int slot = 1;
        for (std::size_t i = 0; i < N; ++i) {
            detail::pushPrimitiveVectorElement<T>(L, static_cast<T>(array[i]));
            lua_rawseti(L, tableIndex, slot++);
        }
    }

    template <class T, std::size_t N>
    void assignStdArray(std::array<T, N>& dest, std::array<T, N> src) {
        for (std::size_t i = 0; i < N; ++i) {
            if constexpr (std::is_same_v<T, bool>) {
                dest[i] = src[i];
            } else {
                dest[i] = std::move(src[i]);
            }
        }
    }

    template <class T, std::size_t N, class U>
    void assignStdArray(U (&dest)[N], std::array<T, N> src) {
        for (std::size_t i = 0; i < N; ++i) {
            if constexpr (std::is_same_v<T, bool>) {
                dest[i] = static_cast<U>(src[i]);
            } else {
                dest[i] = static_cast<U>(std::move(src[i]));
            }
        }
    }

    template <class K>
    inline K checkMapKey(lua_State* L, int idx, char const* label) {
        return detail::checkPrimitiveVectorElement<K>(L, idx, label);
    }

    template <class F, class S>
    inline std::pair<F, S> checkMapValue(lua_State* L, int idx, char const* label) {
        return checkPair<F, S>(L, idx, label);
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

    template <class F, class S>
    inline void pushMapValue(lua_State* L, std::pair<F, S> const& value) {
        pushPair(L, value);
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
        template <class K1, class K2, class V, class Map>
        Map checkPairKeyAssociativeMap(lua_State* L, int idx, char const* label) {
            idx = lua_absindex(L, idx);
            luaL_checktype(L, idx, LUA_TTABLE);
            auto len = static_cast<lua_Integer>(lua_objlen(L, idx));
            if (len < 0) {
                luaL_error(L, "%s: invalid map length", label);
            }
            Map out;
            for (lua_Integer i = 1; i <= len; ++i) {
                lua_rawgeti(L, idx, i);
                requireTableField(L, -1, "first", label);
                K1 keyFirst = checkMapKey<K1>(L, -1, label);
                lua_pop(L, 1);
                requireTableField(L, -1, "second", label);
                K2 keySecond = checkMapKey<K2>(L, -1, label);
                lua_pop(L, 1);
                requireTableField(L, -1, "value", label);
                V value = checkMapValue<V>(L, -1, label);
                lua_pop(L, 2);
                out.emplace(std::make_pair(std::move(keyFirst), std::move(keySecond)), std::move(value));
            }
            return out;
        }

        template <class K1, class K2, class V, class Map>
        void pushPairKeyAssociativeMap(lua_State* L, Map const& map) {
            lua_createtable(L, static_cast<int>(map.size()), 0);
            int tableIndex = lua_gettop(L);
            int i = 1;
            for (auto const& entry : map) {
                lua_createtable(L, 0, 3);
                int entryIndex = lua_gettop(L);
                pushMapKey(L, entry.first.first);
                lua_setfield(L, entryIndex, "first");
                pushMapKey(L, entry.first.second);
                lua_setfield(L, entryIndex, "second");
                pushMapValue(L, entry.second);
                lua_setfield(L, entryIndex, "value");
                lua_rawseti(L, tableIndex, i++);
            }
        }

        template <class Map>
        void assignPairKeyAssociativeMap(Map& dest, Map src) {
            dest.clear();
            for (auto& entry : src) {
                dest.emplace(std::move(entry.first), std::move(entry.second));
            }
        }
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
                out.insert(checkMapValue<T>(L, -1, label));
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
                pushMapValue(L, elem);
                lua_rawseti(L, tableIndex, i++);
            }
        }
    }

    template <class K1, class K2, class V>
    gd::map<std::pair<K1, K2>, V> checkPairKeyMap(lua_State* L, int idx, char const* label) {
        return detail::checkPairKeyAssociativeMap<K1, K2, V, gd::map<std::pair<K1, K2>, V>>(
            L, idx, label
        );
    }

    template <class K1, class K2, class V>
    gd::unordered_map<std::pair<K1, K2>, V> checkUnorderedPairKeyMap(
        lua_State* L, int idx, char const* label
    ) {
        return detail::checkPairKeyAssociativeMap<
            K1, K2, V, gd::unordered_map<std::pair<K1, K2>, V>>(L, idx, label);
    }

    template <class K, class V>
    gd::map<K, V> checkMap(lua_State* L, int idx, char const* label) {
        return detail::checkAssociativeMap<K, V, gd::map<K, V>>(L, idx, label);
    }

    template <class K, class V>
    gd::unordered_map<K, V> checkUnorderedMap(lua_State* L, int idx, char const* label) {
        return detail::checkAssociativeMap<K, V, gd::unordered_map<K, V>>(L, idx, label);
    }

    template <class K1, class K2, class V>
    void pushPairKeyMap(lua_State* L, gd::map<std::pair<K1, K2>, V> const& map) {
        detail::pushPairKeyAssociativeMap<K1, K2, V, gd::map<std::pair<K1, K2>, V>>(L, map);
    }

    template <class K1, class K2, class V>
    void pushUnorderedPairKeyMap(
        lua_State* L, gd::unordered_map<std::pair<K1, K2>, V> const& map
    ) {
        detail::pushPairKeyAssociativeMap<
            K1, K2, V, gd::unordered_map<std::pair<K1, K2>, V>>(L, map);
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

    namespace detail {
        template <class Map>
        void assignAssociativeMap(Map& dest, Map src) {
            dest.clear();
            for (auto& entry : src) {
                dest[std::move(entry.first)] = std::move(entry.second);
            }
        }

        template <class Set>
        void assignSetContainer(Set& dest, Set src) {
            dest.clear();
            for (auto& elem : src) {
                dest.insert(std::move(elem));
            }
        }
    }

    template <class K1, class K2, class V>
    void assignPairKeyMap(gd::map<std::pair<K1, K2>, V>& dest, gd::map<std::pair<K1, K2>, V> src) {
        detail::assignPairKeyAssociativeMap(dest, std::move(src));
    }

    template <class K1, class K2, class V>
    void assignUnorderedPairKeyMap(
        gd::unordered_map<std::pair<K1, K2>, V>& dest,
        gd::unordered_map<std::pair<K1, K2>, V> src
    ) {
        detail::assignPairKeyAssociativeMap(dest, std::move(src));
    }

    template <class K, class V>
    void assignMap(gd::map<K, V>& dest, gd::map<K, V> src) {
        detail::assignAssociativeMap(dest, std::move(src));
    }

    template <class K, class V>
    void assignUnorderedMap(gd::unordered_map<K, V>& dest, gd::unordered_map<K, V> src) {
        detail::assignAssociativeMap(dest, std::move(src));
    }

    template <class T>
    void assignSet(gd::set<T>& dest, gd::set<T> src) {
        detail::assignSetContainer(dest, std::move(src));
    }

    template <class T>
    void assignUnorderedSet(gd::unordered_set<T>& dest, gd::unordered_set<T> src) {
        detail::assignSetContainer(dest, std::move(src));
    }

    template <class T>
    void assignPrimitiveVector(gd::vector<T>& dest, gd::vector<T> src) {
        dest.clear();
        dest.reserve(src.size());
        if constexpr (std::is_same_v<T, bool>) {
            for (std::size_t i = 0; i < src.size(); ++i) {
                dest.push_back(src[i]);
            }
        } else {
            for (auto& elem : src) {
                dest.push_back(std::move(elem));
            }
        }
    }
}
