#pragma once

#include "framework/stack/Stack.hpp"
#include "framework/stack/Types.hpp"
#include "framework/usertype/Usertype.hpp"
#include "framework/view/ReadOnlyVectorView.hpp"

#include <Geode/Geode.hpp>
#include <array>
#include <cstdint>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <type_traits>
#include <utility>

namespace luax {
    template <class F, class S>
    std::pair<F, S> checkPair(lua_State* L, int idx, char const* label);

    template <class F, class S>
    void pushPair(lua_State* L, std::pair<F, S> const& pair);
} // namespace luax

namespace luax::detail {
    template <class T>
    struct is_std_pair : std::false_type {};

    template <class F, class S>
    struct is_std_pair<std::pair<F, S>> : std::true_type {};

    template <class T>
    inline constexpr bool is_std_pair_v = is_std_pair<T>::value;

    template <class T>
    struct is_gd_vector : std::false_type {};

    template <class T>
    struct is_gd_vector<gd::vector<T>> : std::true_type {};

    template <class T>
    inline constexpr bool is_gd_vector_v = is_gd_vector<T>::value;

    template <class V>
    struct gd_vector_element;

    template <class T>
    struct gd_vector_element<gd::vector<T>> {
        using type = T;
    };

    template <class V>
    using gd_vector_element_t = typename gd_vector_element<V>::type;

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
        }
        else if constexpr (std::is_same_v<T, float>) {
            return check<float>(L, idx, label);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return check<double>(L, idx, label);
        }
        else if constexpr (std::is_same_v<T, int>) {
            return check<int>(L, idx, label);
        }
        else if constexpr (std::is_same_v<T, unsigned>) {
            return check<unsigned>(L, idx, label);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return check<std::string>(L, idx, label);
        }
        else if constexpr (std::is_same_v<T, gd::string>) {
            auto storage = check<std::string>(L, idx, label);
            return gd::string(storage.c_str());
        }
        else if constexpr (std::is_enum_v<T>) {
            return static_cast<T>(static_cast<int>(check<int>(L, idx, label)));
        }
        else if constexpr (std::is_integral_v<T>) {
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
        }
        else if constexpr (is_std_pair_v<T>) {
            return luax::checkPair<typename T::first_type, typename T::second_type>(L, idx, label);
        }
        else {
            return check<T>(L, idx, label);
        }
    }

    template <class T>
    inline void pushPrimitiveVectorElement(lua_State* L, T const& value) {
        if constexpr (std::is_same_v<T, bool>) {
            push(L, value);
        }
        else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            push(L, value);
        }
        else if constexpr (std::is_same_v<T, int>) {
            lua_pushnumber(L, static_cast<double>(value));
        }
        else if constexpr (std::is_same_v<T, unsigned>) {
            push(L, value);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            push(L, value);
        }
        else if constexpr (std::is_same_v<T, gd::string>) {
            push(L, std::string(value.c_str()));
        }
        else if constexpr (std::is_enum_v<T>) {
            lua_pushnumber(L, static_cast<double>(static_cast<int>(value)));
        }
        else if constexpr (std::is_integral_v<T>) {
            if constexpr (sizeof(T) > sizeof(int) || !std::is_signed_v<T>) {
                pushIntegerString(L, value);
            }
            else {
                lua_pushnumber(L, static_cast<double>(value));
            }
        }
        else if constexpr (is_std_pair_v<T>) {
            luax::pushPair<typename T::first_type, typename T::second_type>(L, value);
        }
        else {
            push(L, value);
        }
    }

    template <class T, class PushFn>
    inline void pushViaPointer(lua_State* L, T* ptr, PushFn&& pushValue) {
        if (ptr == nullptr) {
            lua_pushnil(L);
            return;
        }
        pushValue(L, *ptr);
    }

    template <class T, class PushFn>
    inline void pushViaPointer(lua_State* L, T const* ptr, PushFn&& pushValue) {
        pushViaPointer(L, const_cast<T*>(ptr), std::forward<PushFn>(pushValue));
    }

    template <class PushElemFn>
    inline void pushIndexedTable(lua_State* L, std::size_t size, PushElemFn&& pushElem) {
        lua_createtable(L, static_cast<int>(size), 0);
        int tableIndex = lua_gettop(L);
        for (std::size_t i = 0; i < size; ++i) {
            pushElem(L, i);
            lua_rawseti(L, tableIndex, static_cast<int>(i + 1));
        }
    }

    template <class T, class Dest, class Src>
    inline void assignContainer(Dest& dest, Src& src, std::size_t count) {
        using DestElem = std::decay_t<decltype(dest[0])>;
        for (std::size_t i = 0; i < count; ++i) {
            if constexpr (std::is_same_v<T, bool>) {
                dest[i] = static_cast<DestElem>(src[i]);
            }
            else {
                dest[i] = static_cast<DestElem>(std::move(src[i]));
            }
        }
    }

    inline int absCheckIndexedTable(lua_State* L, int idx) {
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);
        return idx;
    }

    inline lua_Integer indexedTableLength(lua_State* L, int absIdx) {
        return static_cast<lua_Integer>(lua_objlen(L, absIdx));
    }

    inline void requireNonNegativeIndexedLength(
        lua_State* L, lua_Integer len, char const* label, char const* kind
    ) {
        if (len < 0) {
            luaL_error(L, "%s: invalid %s length", label, kind);
        }
    }

    inline void requireExactIndexedLength(
        lua_State* L, lua_Integer len, std::size_t expected, char const* label
    ) {
        if (len != static_cast<lua_Integer>(expected)) {
            luaL_error(
                L, "%s: expected array length %zu, got %lld", label, expected, static_cast<long long>(len)
            );
        }
    }

    template <class CheckElemFn>
    inline void checkIndexedTableElements(
        lua_State* L, int absIdx, lua_Integer len, CheckElemFn&& checkElem
    ) {
        for (lua_Integer i = 1; i <= len; ++i) {
            lua_rawgeti(L, absIdx, i);
            checkElem(L, i);
            lua_pop(L, 1);
        }
    }
} // namespace luax::detail

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
        detail::pushViaPointer(L, pair, [](lua_State* state, std::pair<F, S> const& value) {
            pushPair(state, value);
        });
    }

    template <class F, class S>
    void pushPair(lua_State* L, std::pair<F, S> const* pair) {
        detail::pushViaPointer(L, pair, [](lua_State* state, std::pair<F, S> const& value) {
            pushPair(state, value);
        });
    }

    template <class T>
    gd::vector<T> checkPrimitiveVector(lua_State* L, int idx, char const* label) {
        idx = detail::absCheckIndexedTable(L, idx);
        auto len = detail::indexedTableLength(L, idx);
        detail::requireNonNegativeIndexedLength(L, len, label, "vector");
        gd::vector<T> out;
        out.reserve(static_cast<std::size_t>(len));
        detail::checkIndexedTableElements(L, idx, len, [&](lua_State* state, lua_Integer) {
            out.push_back(detail::checkPrimitiveVectorElement<T>(state, -1, label));
        });
        return out;
    }

    template <class T>
    void pushPrimitiveVector(lua_State* L, gd::vector<T> const& vector) {
        detail::pushIndexedTable(L, vector.size(), [&](lua_State* state, std::size_t i) {
            detail::pushPrimitiveVectorElement(state, vector[i]);
        });
    }

    template <class T>
    void pushPrimitiveVector(lua_State* L, gd::vector<T>* vector) {
        detail::pushViaPointer(L, vector, [](lua_State* state, gd::vector<T> const& value) {
            pushPrimitiveVector(state, value);
        });
    }

    template <class T>
    void pushPrimitiveVector(lua_State* L, gd::vector<T> const* vector) {
        detail::pushViaPointer(L, vector, [](lua_State* state, gd::vector<T> const& value) {
            pushPrimitiveVector(state, value);
        });
    }

    template <class T, std::size_t N>
    std::array<T, N> checkStdArray(lua_State* L, int idx, char const* label) {
        idx = detail::absCheckIndexedTable(L, idx);
        auto len = detail::indexedTableLength(L, idx);
        detail::requireExactIndexedLength(L, len, N, label);
        std::array<T, N> out{};
        detail::checkIndexedTableElements(L, idx, len, [&](lua_State* state, lua_Integer i) {
            out[static_cast<std::size_t>(i - 1)] =
                detail::checkPrimitiveVectorElement<T>(state, -1, label);
        });
        return out;
    }

    template <class T, std::size_t N>
    void pushStdArray(lua_State* L, std::array<T, N> const& array) {
        detail::pushIndexedTable(L, N, [&](lua_State* state, std::size_t i) {
            detail::pushPrimitiveVectorElement(state, array[i]);
        });
    }

    template <class T, std::size_t N>
    void pushStdArray(lua_State* L, std::array<T, N>* array) {
        detail::pushViaPointer(L, array, [](lua_State* state, std::array<T, N> const& value) {
            pushStdArray<T, N>(state, value);
        });
    }

    template <class T, std::size_t N>
    void pushStdArray(lua_State* L, std::array<T, N> const* array) {
        detail::pushViaPointer(L, array, [](lua_State* state, std::array<T, N> const& value) {
            pushStdArray<T, N>(state, value);
        });
    }

    template <class T, std::size_t N, class U>
    void pushStdArray(lua_State* L, U const (&array)[N]) {
        detail::pushIndexedTable(L, N, [&](lua_State* state, std::size_t i) {
            detail::pushPrimitiveVectorElement<T>(state, static_cast<T>(array[i]));
        });
    }

    template <class T, std::size_t N>
    void assignStdArray(std::array<T, N>& dest, std::array<T, N> src) {
        detail::assignContainer<T>(dest, src, N);
    }

    template <class T, std::size_t N, class U>
    void assignStdArray(U (&dest)[N], std::array<T, N> src) {
        detail::assignContainer<T>(dest, src, N);
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
        if constexpr (detail::is_gd_vector_v<V>) {
            using Elem = detail::gd_vector_element_t<V>;
            if constexpr (std::is_pointer_v<Elem>) {
                using Pointee = std::remove_pointer_t<Elem>;
                if constexpr (std::is_base_of_v<cocos2d::CCObject, Pointee>) {
                    return checkObjectVectorView<Pointee>(L, idx, label);
                }
                return checkOpaqueVectorView<Pointee>(L, idx, label);
            }
            return checkPrimitiveVector<Elem>(L, idx, label);
        }
        else if constexpr (std::is_pointer_v<V>) {
            using Pointee = std::remove_pointer_t<V>;
            if (lua_isnil(L, idx)) {
                return nullptr;
            }
            return Usertype<Pointee>::check(L, idx, label);
        }
        else {
            return detail::checkPrimitiveVectorElement<V>(L, idx, label);
        }
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
        if constexpr (detail::is_gd_vector_v<V>) {
            using Elem = detail::gd_vector_element_t<V>;
            if constexpr (std::is_pointer_v<Elem>) {
                using Pointee = std::remove_pointer_t<Elem>;
                detail::pushIndexedTable(L, value.size(), [&](lua_State* state, std::size_t i) {
                    auto* elem = value[i];
                    if constexpr (std::is_base_of_v<cocos2d::CCObject, Pointee>) {
                        if (elem == nullptr) {
                            lua_pushnil(state);
                        }
                        else if constexpr (std::is_same_v<Pointee, cocos2d::CCObject>) {
                            Usertype<cocos2d::CCObject>::pushBorrowedDynamic(state, elem);
                        }
                        else {
                            Usertype<Pointee>::pushBorrowed(state, elem);
                        }
                    }
                    else if (elem == nullptr) {
                        lua_pushnil(state);
                    }
                    else {
                        pushOpaqueHandle(state, elem);
                    }
                });
            }
            else {
                pushPrimitiveVector<Elem>(L, value);
            }
        }
        else if constexpr (std::is_pointer_v<V>) {
            using Pointee = std::remove_pointer_t<V>;
            if (value == nullptr) {
                lua_pushnil(L);
            }
            else if constexpr (std::is_same_v<Pointee, cocos2d::CCObject>) {
                Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, value);
            }
            else {
                Usertype<Pointee>::pushBorrowed(L, value);
            }
        }
        else {
            detail::pushPrimitiveVectorElement(L, value);
        }
    }

    template <class T>
    void pushNestedPrimitiveVectorPointers(lua_State* L, gd::vector<gd::vector<T>*> const& outer) {
        detail::pushIndexedTable(L, outer.size(), [&](lua_State* state, std::size_t i) {
            auto* inner = outer[i];
            if (inner == nullptr) {
                lua_createtable(state, 0, 0);
            }
            else {
                pushPrimitiveVector<T>(state, *inner);
            }
        });
    }

    template <class T>
    void pushNestedPrimitiveVectorPointers(lua_State* L, gd::vector<gd::vector<T>*>* outer) {
        detail::pushViaPointer(L, outer, [](lua_State* state, gd::vector<gd::vector<T>*> const& value) {
            pushNestedPrimitiveVectorPointers<T>(state, value);
        });
    }

    template <class T>
    void pushNestedPrimitiveVectorPointers(lua_State* L, gd::vector<gd::vector<T>*> const* outer) {
        detail::pushViaPointer(L, outer, [](lua_State* state, gd::vector<gd::vector<T>*> const& value) {
            pushNestedPrimitiveVectorPointers<T>(state, value);
        });
    }

    namespace detail {
        template <class K1, class K2, class V, class Map>
        Map checkPairKeyAssociativeMap(lua_State* L, int idx, char const* label) {
            idx = absCheckIndexedTable(L, idx);
            auto len = indexedTableLength(L, idx);
            requireNonNegativeIndexedLength(L, len, label, "map");
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
                out.emplace(
                    std::make_pair(std::move(keyFirst), std::move(keySecond)), std::move(value)
                );
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
            idx = absCheckIndexedTable(L, idx);
            auto len = indexedTableLength(L, idx);
            requireNonNegativeIndexedLength(L, len, label, "set");
            Set out;
            checkIndexedTableElements(L, idx, len, [&](lua_State* state, lua_Integer) {
                out.insert(checkMapValue<T>(state, -1, label));
            });
            return out;
        }

        template <class T, class Set>
        void pushSetAsTable(lua_State* L, Set const& set) {
            auto it = set.begin();
            pushIndexedTable(L, set.size(), [&](lua_State* state, std::size_t) {
                pushMapValue(state, *it++);
            });
        }

        template <class Map>
        void pushAssociativeMapPointer(lua_State* L, Map* map) {
            pushViaPointer(L, map, [](lua_State* state, Map const& value) {
                pushAssociativeMap<typename Map::key_type, typename Map::mapped_type, Map>(
                    state, value
                );
            });
        }

        template <class Map>
        void pushAssociativeMapPointer(lua_State* L, Map const* map) {
            pushAssociativeMapPointer(L, const_cast<Map*>(map));
        }

        template <class Set>
        void pushSetContainerPointer(lua_State* L, Set* set) {
            pushViaPointer(L, set, [](lua_State* state, Set const& value) {
                pushSetAsTable<typename Set::value_type, Set>(state, value);
            });
        }

        template <class Set>
        void pushSetContainerPointer(lua_State* L, Set const* set) {
            pushSetContainerPointer(L, const_cast<Set*>(set));
        }

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
    } // namespace detail

    template <class T>
    void assignPrimitiveVector(gd::vector<T>& dest, gd::vector<T> src) {
        dest.clear();
        dest.reserve(src.size());
        if constexpr (std::is_same_v<T, bool>) {
            for (std::size_t i = 0; i < src.size(); ++i) {
                dest.push_back(src[i]);
            }
        }
        else {
            for (auto& elem : src) {
                dest.push_back(std::move(elem));
            }
        }
    }
} // namespace luax
