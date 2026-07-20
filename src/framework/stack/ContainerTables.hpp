#pragma once

#include "framework/stack/Stack.hpp"
#include "framework/stack/Types.hpp"
#include "framework/usertype/Usertype.hpp"
#include "framework/view/ReadOnlyVectorView.hpp"

#include <Geode/Geode.hpp>
#include <array>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace luax {
    template <class T>
    void checkContainerValue(lua_State* L, int idx, char const* label, T& out);

    template <class T>
    T checkContainerValue(lua_State* L, int idx, char const* label);

    template <class T>
    void pushContainerValue(lua_State* L, T const& value);

    template <class T>
    void pushContainerValue(lua_State* L, T const* value);

    template <class T>
    void pushContainerValue(lua_State* L, T const& value, cocos2d::CCObject* owner);

    template <class T>
    void pushContainerValue(lua_State* L, T const* value, cocos2d::CCObject* owner);

    template <class Array, class U, std::size_t N>
    void pushContainerValue(lua_State* L, U const (&value)[N]);

    template <class Array, class U, std::size_t N>
    void pushContainerValue(lua_State* L, U const (&value)[N], cocos2d::CCObject* owner);

    template <class T>
    void assignContainerValue(T& dest, T const& source);

    template <class T>
    void assignContainerValue(T& dest, T&& source);

    template <class T, std::size_t N, class U>
    void assignContainerValue(U (&dest)[N], std::array<T, N> source);

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

    template <class T>
    struct is_std_array : std::false_type {};

    template <class T, std::size_t N>
    struct is_std_array<std::array<T, N>> : std::true_type {};

    template <class T>
    inline constexpr bool is_std_array_v = is_std_array<std::remove_cv_t<T>>::value;

    template <class T>
    struct is_gd_map : std::false_type {};

    template <class K, class V>
    struct is_gd_map<gd::map<K, V>> : std::true_type {};

    template <class T>
    struct is_gd_unordered_map : std::false_type {};

    template <class K, class V>
    struct is_gd_unordered_map<gd::unordered_map<K, V>> : std::true_type {};

    template <class T>
    inline constexpr bool is_associative_map_v =
        is_gd_map<std::remove_cv_t<T>>::value || is_gd_unordered_map<std::remove_cv_t<T>>::value;

    template <class T>
    struct is_gd_set : std::false_type {};

    template <class V>
    struct is_gd_set<gd::set<V>> : std::true_type {};

    template <class T>
    struct is_gd_unordered_set : std::false_type {};

    template <class V>
    struct is_gd_unordered_set<gd::unordered_set<V>> : std::true_type {};

    template <class T>
    inline constexpr bool is_associative_set_v =
        is_gd_set<std::remove_cv_t<T>>::value || is_gd_unordered_set<std::remove_cv_t<T>>::value;

    template <class T>
    struct is_std_tuple : std::false_type {};

    template <class... Ts>
    struct is_std_tuple<std::tuple<Ts...>> : std::true_type {};

    template <class T>
    inline constexpr bool is_std_tuple_v = is_std_tuple<std::remove_cv_t<T>>::value;

    template <class T>
    inline constexpr bool is_wide_container_integer_v = std::is_same_v<std::remove_cv_t<T>, long> ||
        std::is_same_v<std::remove_cv_t<T>, unsigned long> ||
        std::is_same_v<std::remove_cv_t<T>, long long> ||
        std::is_same_v<std::remove_cv_t<T>, unsigned long long> ||
        (std::is_integral_v<std::remove_cv_t<T>> && sizeof(T) > sizeof(int));

    template <class T>
    inline constexpr bool is_container_composite_v =
        is_gd_vector_v<std::remove_cv_t<T>> || is_std_array_v<T> || is_associative_map_v<T> ||
        is_associative_set_v<T> || is_std_pair_v<std::remove_cv_t<T>> || is_std_tuple_v<T>;

    template <class T>
    inline constexpr bool is_ccobject_pointee_v = !std::is_void_v<std::remove_cv_t<T>> &&
        requires(std::remove_cv_t<T>* ptr) { static_cast<cocos2d::CCObject*>(ptr); };

    template <class T>
    struct audited_pointer_grid_mirror_impl {
        using type = std::remove_cv_t<T>;
    };

    template <class T>
    struct audited_pointer_grid_mirror_impl<gd::vector<T>> {
        using type = gd::vector<typename audited_pointer_grid_mirror_impl<std::remove_cv_t<T>>::type>;
    };

    template <class T>
    struct audited_pointer_grid_mirror_impl<T*> {
        using Pointee = std::remove_cv_t<T>;
        using type = std::conditional_t<
            is_container_composite_v<Pointee>,
            typename audited_pointer_grid_mirror_impl<Pointee>::type, T*>;
    };

    template <class T>
    using audited_pointer_grid_mirror_t =
        typename audited_pointer_grid_mirror_impl<std::remove_cv_t<std::remove_reference_t<T>>>::type;

    template <class T>
    struct has_audited_composite_pointer_impl : std::false_type {};

    template <class T>
    struct has_audited_composite_pointer_impl<gd::vector<T>> :
        has_audited_composite_pointer_impl<std::remove_cv_t<T>> {};

    template <class T>
    struct has_audited_composite_pointer_impl<T*> :
        std::bool_constant<
            is_container_composite_v<std::remove_cv_t<T>> ||
            has_audited_composite_pointer_impl<std::remove_cv_t<T>>::value> {};

    template <class T>
    inline constexpr bool has_audited_composite_pointer_v =
        has_audited_composite_pointer_impl<std::remove_cv_t<std::remove_reference_t<T>>>::value;

    template <class T>
    struct is_audited_pointer_grid_node_impl :
        std::bool_constant<!is_container_composite_v<std::remove_cv_t<T>>> {};

    template <class T>
    struct is_audited_pointer_grid_node_impl<gd::vector<T>> :
        is_audited_pointer_grid_node_impl<std::remove_cv_t<T>> {};

    template <class T>
    struct is_audited_pointer_grid_node_impl<T*> :
        std::bool_constant<
            !is_container_composite_v<std::remove_cv_t<T>> ||
            (is_gd_vector_v<std::remove_cv_t<T>> &&
             is_audited_pointer_grid_node_impl<std::remove_cv_t<T>>::value)> {};

    template <class T>
    inline constexpr bool is_audited_pointer_grid_v =
        is_gd_vector_v<std::remove_cv_t<std::remove_reference_t<T>>> &&
        is_audited_pointer_grid_node_impl<std::remove_cv_t<std::remove_reference_t<T>>>::value &&
        has_audited_composite_pointer_v<T>;

    template <class PushElemFn>
    inline void pushIndexedTable(lua_State* L, std::size_t size, PushElemFn&& pushElem) {
        lua_createtable(L, static_cast<int>(size), 0);
        int tableIndex = lua_gettop(L);
        for (std::size_t i = 0; i < size; ++i) {
            pushElem(L, i);
            lua_rawseti(L, tableIndex, static_cast<int>(i + 1));
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

    template <class T>
    void pushAuditedPointerGridValue(lua_State* L, T const& value, cocos2d::CCObject* owner) {
        using U = std::remove_cv_t<T>;
        if constexpr (std::is_pointer_v<U> && is_container_composite_v<std::remove_cv_t<std::remove_pointer_t<U>>>) {
            if (value == nullptr) {
                lua_createtable(L, 0, 0);
            }
            else {
                pushAuditedPointerGridValue(L, *value, owner);
            }
        }
        else if constexpr (is_gd_vector_v<U> && has_audited_composite_pointer_v<U>) {
            pushIndexedTable(L, value.size(), [&](lua_State* state, std::size_t i) {
                pushAuditedPointerGridValue(state, value[i], owner);
            });
        }
        else {
            pushContainerValue(L, value, owner);
        }
    }

    template <class T>
    void destroyAuditedPointerGridValue(T& value) {
        using U = std::remove_cv_t<T>;
        if constexpr (std::is_pointer_v<U> && is_container_composite_v<std::remove_cv_t<std::remove_pointer_t<U>>>) {
            if (value != nullptr) {
                destroyAuditedPointerGridValue(*value);
                delete value;
                value = nullptr;
            }
        }
        else if constexpr (is_gd_vector_v<U> && has_audited_composite_pointer_v<U>) {
            for (auto& element : value) {
                destroyAuditedPointerGridValue(element);
            }
            value.clear();
        }
    }

    template <class Actual, class Mirror>
    void assignAuditedPointerGridValue(Actual& dest, Mirror& source) {
        using U = std::remove_cv_t<Actual>;
        if constexpr (!has_audited_composite_pointer_v<U>) {
            assignContainerValue(dest, std::move(source));
        }
        else if constexpr (
            std::is_pointer_v<U> &&
            is_container_composite_v<std::remove_cv_t<std::remove_pointer_t<U>>>
        ) {
            using Pointee = std::remove_cv_t<std::remove_pointer_t<U>>;
            if (dest == nullptr) {
                dest = new Pointee();
            }
            assignAuditedPointerGridValue(*dest, source);
        }
        else if constexpr (is_gd_vector_v<U>) {
            using Element = typename U::value_type;
            while (dest.size() > source.size()) {
                destroyAuditedPointerGridValue(dest.back());
                dest.pop_back();
            }
            while (dest.size() < source.size()) {
                dest.push_back(Element{});
            }
            for (std::size_t i = 0; i < source.size(); ++i) {
                assignAuditedPointerGridValue(dest[i], source[i]);
            }
        }
    }

} // namespace luax::detail

namespace luax {
    template <class T>
    void assignOpaqueVectorView(gd::vector<T*>& dest, gd::vector<T*> src);

    template <class Actual>
        requires(!std::is_pointer_v<Actual>)
    void pushAuditedPointerGrid(lua_State* L, Actual const& value, cocos2d::CCObject* owner = nullptr) {
        static_assert(detail::is_audited_pointer_grid_v<Actual>);
        detail::pushAuditedPointerGridValue(L, value, owner);
    }

    template <class Actual>
    void pushAuditedPointerGrid(lua_State* L, Actual const* value, cocos2d::CCObject* owner = nullptr) {
        static_assert(detail::is_audited_pointer_grid_v<Actual>);
        if (value == nullptr) {
            lua_pushnil(L);
        }
        else {
            detail::pushAuditedPointerGridValue(L, *value, owner);
        }
    }

    template <class Actual>
    detail::audited_pointer_grid_mirror_t<Actual> checkAuditedPointerGrid(
        lua_State* L, int idx, char const* label
    ) {
        static_assert(detail::is_audited_pointer_grid_v<Actual>);
        return checkContainerValue<detail::audited_pointer_grid_mirror_t<Actual>>(L, idx, label);
    }

    template <class Actual>
    void assignAuditedPointerGrid(Actual& dest, detail::audited_pointer_grid_mirror_t<Actual>&& source) {
        static_assert(detail::is_audited_pointer_grid_v<Actual>);
        detail::assignAuditedPointerGridValue(dest, source);
    }

    template <class T>
    void assignOpaqueVectorView(gd::vector<T*>& dest, gd::vector<T*> src) {
        assignContainerValue(dest, std::move(src));
    }

} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
    #include "framework/stack/Types.generated.containers.hpp"
#endif

namespace luax::detail {
    template <class T>
    T checkContainerLeaf(lua_State* L, int idx, char const* label) {
        using U = std::remove_cv_t<T>;
        if constexpr (std::is_same_v<U, bool>) {
            return luax::check<bool>(L, idx, label);
        }
        else if constexpr (std::is_floating_point_v<U>) {
            return static_cast<U>(luax::check<double>(L, idx, label));
        }
        else if constexpr (std::is_enum_v<U>) {
            return static_cast<U>(luax::check<int>(L, idx, label));
        }
        else if constexpr (std::is_same_v<U, unsigned>) {
            return luax::check<unsigned>(L, idx, label);
        }
        else if constexpr (std::is_integral_v<U>) {
            if constexpr (is_wide_container_integer_v<U>) {
                return luax::checkIntegerString<U>(L, idx, label);
            }
            else {
                return static_cast<U>(luax::check<int>(L, idx, label));
            }
        }
        else if constexpr (std::is_same_v<U, std::string>) {
            return luax::check<std::string>(L, idx, label);
        }
        else if constexpr (std::is_same_v<U, gd::string>) {
            auto value = luax::check<std::string>(L, idx, label);
            return gd::string(value.c_str());
        }
        else if constexpr (std::is_pointer_v<U>) {
            using Pointee = std::remove_cv_t<std::remove_pointer_t<U>>;
            if (lua_isnil(L, idx)) {
                return nullptr;
            }
            if constexpr (is_ccobject_pointee_v<Pointee>) {
                return Usertype<Pointee>::check(L, idx, label);
            }
            else {
                return checkOpaqueHandle<Pointee>(L, idx, label);
            }
        }
        else {
            return luax::check<U>(L, idx, label);
        }
    }

    template <class T>
    void pushContainerLeaf(lua_State* L, T const& value) {
        using U = std::remove_cv_t<T>;
        if constexpr (std::is_same_v<U, bool>) {
            luax::push(L, value);
        }
        else if constexpr (std::is_floating_point_v<U>) {
            lua_pushnumber(L, static_cast<double>(value));
        }
        else if constexpr (std::is_enum_v<U>) {
            lua_pushnumber(L, static_cast<double>(static_cast<int>(value)));
        }
        else if constexpr (std::is_same_v<U, unsigned>) {
            luax::push(L, value);
        }
        else if constexpr (std::is_integral_v<U>) {
            if constexpr (is_wide_container_integer_v<U>) {
                luax::pushIntegerString(L, value);
            }
            else {
                lua_pushnumber(L, static_cast<double>(value));
            }
        }
        else if constexpr (std::is_same_v<U, std::string>) {
            luax::push(L, value);
        }
        else if constexpr (std::is_same_v<U, gd::string>) {
            luax::push(L, std::string(value.c_str()));
        }
        else if constexpr (std::is_pointer_v<U>) {
            using Pointee = std::remove_cv_t<std::remove_pointer_t<U>>;
            auto* ptr = const_cast<Pointee*>(value);
            if (ptr == nullptr) {
                lua_pushnil(L);
            }
            else if constexpr (is_ccobject_pointee_v<Pointee>) {
                if constexpr (std::is_same_v<Pointee, cocos2d::CCObject>) {
                    Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, ptr);
                }
                else {
                    Usertype<Pointee>::pushBorrowed(L, ptr);
                }
            }
            else {
                pushOpaqueHandle(L, ptr);
            }
        }
        else {
            luax::push(L, value);
        }
    }

    inline void validateFixedIndexedKeys(
        lua_State* L, int idx, std::size_t size, char const* label, char const* kind
    ) {
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);
        requireExactIndexedLength(L, indexedTableLength(L, idx), size, label);
        lua_pushnil(L);
        while (lua_next(L, idx) != 0) {
            int const keyType = lua_type(L, -2);
            if (keyType == LUA_TNUMBER) {
                lua_Number const raw = lua_tonumber(L, -2);
                if (raw < 1 || raw > static_cast<lua_Number>(size) ||
                    static_cast<lua_Number>(static_cast<lua_Integer>(raw)) != raw) {
                    luaL_error(L, "%s: invalid %s index", label, kind);
                }
            }
            lua_pop(L, 1);
        }
    }

    template <class T>
    void checkContainerValueInto(lua_State* L, int idx, char const* label, T& out);

    template <class Tuple, std::size_t... Is>
    void checkTupleValueInto(
        lua_State* L, int idx, char const* label, Tuple& out, std::index_sequence<Is...>
    ) {
        idx = lua_absindex(L, idx);
        validateFixedIndexedKeys(L, idx, sizeof...(Is), label, "tuple");
        (
            [&] {
                lua_rawgeti(L, idx, static_cast<int>(Is + 1));
                checkContainerValueInto(L, -1, label, std::get<Is>(out));
                lua_pop(L, 1);
            }(),
            ...);
    }

    template <class T>
    void checkVectorValueInto(lua_State* L, int idx, char const* label, T& out) {
        using U = std::remove_cv_t<T>;
        using Element = typename U::value_type;
        if constexpr (std::is_pointer_v<Element>) {
            using Pointee = std::remove_cv_t<std::remove_pointer_t<Element>>;
            static_assert(
                !is_container_composite_v<Pointee>,
                "container-pointer descendants require a specialized field helper"
            );
            gd::vector<Pointee*> checked;
            if constexpr (is_ccobject_pointee_v<Pointee>) {
                checked = checkObjectVectorView<Pointee>(L, idx, label);
            }
            else {
                checked = checkOpaqueVectorView<Pointee>(L, idx, label);
            }
            out.clear();
            out.reserve(checked.size());
            for (auto* value : checked) {
                out.push_back(value);
            }
            return;
        }

        idx = absCheckIndexedTable(L, idx);
        auto const len = indexedTableLength(L, idx);
        requireNonNegativeIndexedLength(L, len, label, "vector");
        out.clear();
        out.reserve(static_cast<std::size_t>(len));
        for (lua_Integer i = 1; i <= len; ++i) {
            lua_rawgeti(L, idx, i);
            Element candidate;
            checkContainerValueInto(L, -1, label, candidate);
            lua_pop(L, 1);
            if constexpr (std::is_same_v<Element, bool>) {
                out.push_back(candidate);
            }
            else {
                out.emplace_back(std::move(candidate));
            }
        }
    }

    template <class T>
    void checkMapValueInto(lua_State* L, int idx, char const* label, T& out) {
        using U = std::remove_cv_t<T>;
        using Key = typename U::key_type;
        using Value = typename U::mapped_type;
        out.clear();

        if constexpr (is_std_pair_v<Key>) {
            idx = absCheckIndexedTable(L, idx);
            auto const len = indexedTableLength(L, idx);
            requireNonNegativeIndexedLength(L, len, label, "map");
            for (lua_Integer i = 1; i <= len; ++i) {
                lua_rawgeti(L, idx, i);
                int const entry = absCheckIndexedTable(L, -1);
                Key key{};
                lua_getfield(L, entry, "first");
                checkContainerValueInto(L, -1, label, key.first);
                lua_pop(L, 1);
                lua_getfield(L, entry, "second");
                checkContainerValueInto(L, -1, label, key.second);
                lua_pop(L, 1);
                lua_getfield(L, entry, "value");
                Value candidate;
                checkContainerValueInto(L, -1, label, candidate);
                lua_pop(L, 1);
                if (out.find(key) == out.end()) {
                    out.emplace(std::move(key), std::move(candidate));
                }
                lua_pop(L, 1);
            }
        }
        else {
            idx = lua_absindex(L, idx);
            luaL_checktype(L, idx, LUA_TTABLE);
            lua_pushnil(L);
            while (lua_next(L, idx) != 0) {
                Key key = checkContainerLeaf<Key>(L, -2, label);
                checkContainerValueInto(L, -1, label, out[std::move(key)]);
                lua_pop(L, 1);
            }
        }
    }

    template <class T>
    void checkSetValueInto(lua_State* L, int idx, char const* label, T& out) {
        using U = std::remove_cv_t<T>;
        using Element = typename U::value_type;
        idx = absCheckIndexedTable(L, idx);
        auto const len = indexedTableLength(L, idx);
        requireNonNegativeIndexedLength(L, len, label, "set");
        out.clear();
        for (lua_Integer i = 1; i <= len; ++i) {
            lua_rawgeti(L, idx, i);
            Element candidate;
            checkContainerValueInto(L, -1, label, candidate);
            lua_pop(L, 1);
            if (out.find(candidate) == out.end()) {
                out.emplace(std::move(candidate));
            }
        }
    }

    template <class T>
    void checkContainerValueInto(lua_State* L, int idx, char const* label, T& out) {
        using U = std::remove_cv_t<T>;
        static_assert(
            !(std::is_pointer_v<U> &&
              is_container_composite_v<std::remove_cv_t<std::remove_pointer_t<U>>>),
            "container-pointer descendants require a specialized field helper"
        );
        if constexpr (is_gd_vector_v<U>) {
            checkVectorValueInto(L, idx, label, out);
        }
        else if constexpr (is_std_array_v<U>) {
            idx = lua_absindex(L, idx);
            validateFixedIndexedKeys(L, idx, out.size(), label, "array");
            for (std::size_t i = 0; i < out.size(); ++i) {
                lua_rawgeti(L, idx, static_cast<int>(i + 1));
                checkContainerValueInto(L, -1, label, out[i]);
                lua_pop(L, 1);
            }
        }
        else if constexpr (is_associative_map_v<U>) {
            checkMapValueInto(L, idx, label, out);
        }
        else if constexpr (is_associative_set_v<U>) {
            checkSetValueInto(L, idx, label, out);
        }
        else if constexpr (is_std_pair_v<U>) {
            idx = lua_absindex(L, idx);
            luaL_checktype(L, idx, LUA_TTABLE);
            lua_getfield(L, idx, "first");
            checkContainerValueInto(L, -1, label, out.first);
            lua_pop(L, 1);
            lua_getfield(L, idx, "second");
            checkContainerValueInto(L, -1, label, out.second);
            lua_pop(L, 1);
        }
        else if constexpr (is_std_tuple_v<U>) {
            checkTupleValueInto(L, idx, label, out, std::make_index_sequence<std::tuple_size_v<U>>{});
        }
        else {
            auto const value = checkContainerLeaf<U>(L, idx, label);
            out = value;
        }
    }

    template <class T>
    void pushContainerValueImpl(lua_State* L, T const& value, cocos2d::CCObject* owner, bool ownerAware);

    template <class Tuple, std::size_t... Is>
    void pushTupleValueImpl(
        lua_State* L, Tuple const& value, cocos2d::CCObject* owner, bool ownerAware,
        std::index_sequence<Is...>
    ) {
        lua_createtable(L, static_cast<int>(sizeof...(Is)), 0);
        int const table = lua_gettop(L);
        (
            [&] {
                pushContainerValueImpl(L, std::get<Is>(value), owner, ownerAware);
                lua_rawseti(L, table, static_cast<int>(Is + 1));
            }(),
            ...);
    }

    template <class T>
    void pushContainerValueImpl(lua_State* L, T const& value, cocos2d::CCObject* owner, bool ownerAware) {
        using U = std::remove_cv_t<T>;
        static_assert(
            !(std::is_pointer_v<U> &&
              is_container_composite_v<std::remove_cv_t<std::remove_pointer_t<U>>>),
            "container-pointer descendants require a specialized field helper"
        );
        if constexpr (is_gd_vector_v<U>) {
            using Element = typename U::value_type;
            if constexpr (std::is_pointer_v<Element>) {
                using Pointee = std::remove_cv_t<std::remove_pointer_t<Element>>;
                static_assert(
                    !is_container_composite_v<Pointee>,
                    "container-pointer descendants require a specialized field helper"
                );
                if (owner != nullptr && ownerAware) {
                    if constexpr (is_ccobject_pointee_v<Pointee>) {
                        pushReadOnlyVectorView<Pointee>(L, value, owner);
                    }
                    else {
                        pushReadOnlyOpaqueVectorView<Pointee>(L, value, owner);
                    }
                    return;
                }
            }
            pushIndexedTable(L, value.size(), [&](lua_State* state, std::size_t i) {
                if constexpr (std::is_same_v<Element, bool>) {
                    bool const element = value[i];
                    pushContainerValueImpl(state, element, owner, ownerAware);
                }
                else {
                    pushContainerValueImpl(state, value[i], owner, ownerAware);
                }
            });
        }
        else if constexpr (is_std_array_v<U>) {
            pushIndexedTable(L, value.size(), [&](lua_State* state, std::size_t i) {
                pushContainerValueImpl(state, value[i], owner, ownerAware);
            });
        }
        else if constexpr (is_associative_map_v<U>) {
            using Key = typename U::key_type;
            if constexpr (is_std_pair_v<Key>) {
                lua_createtable(L, static_cast<int>(value.size()), 0);
                int const table = lua_gettop(L);
                int i = 1;
                for (auto const& entry : value) {
                    lua_createtable(L, 0, 3);
                    int const item = lua_gettop(L);
                    pushContainerValueImpl(L, entry.first.first, owner, ownerAware);
                    lua_setfield(L, item, "first");
                    pushContainerValueImpl(L, entry.first.second, owner, ownerAware);
                    lua_setfield(L, item, "second");
                    pushContainerValueImpl(L, entry.second, owner, ownerAware);
                    lua_setfield(L, item, "value");
                    lua_rawseti(L, table, i++);
                }
            }
            else {
                lua_createtable(L, 0, static_cast<int>(value.size()));
                int const table = lua_gettop(L);
                for (auto const& entry : value) {
                    pushContainerLeaf(L, entry.first);
                    pushContainerValueImpl(L, entry.second, owner, ownerAware);
                    lua_rawset(L, table);
                }
            }
        }
        else if constexpr (is_associative_set_v<U>) {
            lua_createtable(L, static_cast<int>(value.size()), 0);
            int const table = lua_gettop(L);
            int i = 1;
            for (auto const& element : value) {
                pushContainerValueImpl(L, element, owner, ownerAware);
                lua_rawseti(L, table, i++);
            }
        }
        else if constexpr (is_std_pair_v<U>) {
            lua_createtable(L, 0, 2);
            int const table = lua_gettop(L);
            pushContainerValueImpl(L, value.first, owner, ownerAware);
            lua_setfield(L, table, "first");
            pushContainerValueImpl(L, value.second, owner, ownerAware);
            lua_setfield(L, table, "second");
        }
        else if constexpr (is_std_tuple_v<U>) {
            pushTupleValueImpl(
                L, value, owner, ownerAware, std::make_index_sequence<std::tuple_size_v<U>>{}
            );
        }
        else {
            pushContainerLeaf(L, value);
        }
    }

    template <class Expected, class Actual>
    void pushAdaptedContainerValue(
        lua_State* L, Actual const& value, cocos2d::CCObject* owner, bool ownerAware
    ) {
        using ExpectedValue = std::remove_cv_t<Expected>;
        using ActualValue = std::remove_reference_t<Actual>;
        if constexpr (is_std_array_v<ExpectedValue> && std::is_array_v<ActualValue>) {
            static_assert(std::tuple_size_v<ExpectedValue> == std::extent_v<ActualValue>);
            using Element = typename ExpectedValue::value_type;
            pushIndexedTable(L, std::tuple_size_v<ExpectedValue>, [&](lua_State* state, std::size_t i) {
                pushAdaptedContainerValue<Element>(state, value[i], owner, ownerAware);
            });
        }
        else if constexpr (std::is_same_v<std::remove_cv_t<ActualValue>, ExpectedValue>) {
            pushContainerValueImpl(L, value, owner, ownerAware);
        }
        else {
            ExpectedValue converted = static_cast<ExpectedValue>(value);
            pushContainerValueImpl(L, converted, owner, ownerAware);
        }
    }
} // namespace luax::detail

namespace luax {
    template <class T>
    void pushContainerValue(lua_State* L, T const& value) {
        detail::pushContainerValueImpl(L, value, nullptr, false);
    }

    template <class T>
    void pushContainerValue(lua_State* L, T const* value) {
        if (value == nullptr) {
            lua_pushnil(L);
            return;
        }
        detail::pushContainerValueImpl(L, *value, nullptr, true);
    }

    template <class T>
    void pushContainerValue(lua_State* L, T const& value, cocos2d::CCObject* owner) {
        detail::pushContainerValueImpl(L, value, owner, false);
    }

    template <class T>
    void pushContainerValue(lua_State* L, T const* value, cocos2d::CCObject* owner) {
        if (value == nullptr) {
            lua_pushnil(L);
            return;
        }
        detail::pushContainerValueImpl(L, *value, owner, true);
    }

    template <class Array, class U, std::size_t N>
    void pushContainerValue(lua_State* L, U const (&value)[N]) {
        static_assert(detail::is_std_array_v<Array>);
        static_assert(std::tuple_size_v<Array> == N);
        detail::pushAdaptedContainerValue<Array>(L, value, nullptr, false);
    }

    template <class Array, class U, std::size_t N>
    void pushContainerValue(lua_State* L, U const (&value)[N], cocos2d::CCObject* owner) {
        static_assert(detail::is_std_array_v<Array>);
        static_assert(std::tuple_size_v<Array> == N);
        detail::pushAdaptedContainerValue<Array>(L, value, owner, false);
    }
} // namespace luax

namespace luax::detail {
    template <class T, class Source>
    void assignContainerValueFrom(T& dest, Source& source);

    template <class T, class Source, std::size_t... Is>
    void assignTupleValueFrom(T& dest, Source& source, std::index_sequence<Is...>) {
        (assignContainerValueFrom(std::get<Is>(dest), std::get<Is>(source)), ...);
    }

    template <class T, class Source>
    void assignContainerValueFrom(T& dest, Source& source) {
        using U = std::remove_cv_t<T>;
        if constexpr (is_gd_vector_v<U>) {
            using Element = typename U::value_type;
            dest.clear();
            dest.reserve(source.size());
            for (std::size_t i = 0; i < source.size(); ++i) {
                if constexpr (std::is_same_v<Element, bool>) {
                    dest.push_back(static_cast<bool>(source[i]));
                }
                else {
                    dest.emplace_back();
                    assignContainerValueFrom(dest.back(), source[i]);
                }
            }
        }
        else if constexpr (is_std_array_v<U>) {
            for (std::size_t i = 0; i < dest.size(); ++i) {
                assignContainerValueFrom(dest[i], source[i]);
            }
        }
        else if constexpr (is_associative_map_v<U>) {
            dest.clear();
            for (auto& entry : source) {
                assignContainerValueFrom(dest[entry.first], entry.second);
            }
        }
        else if constexpr (is_associative_set_v<U>) {
            using Element = typename U::value_type;
            dest.clear();
            for (auto const& element : source) {
                Element candidate;
                assignContainerValueFrom(candidate, element);
                dest.emplace(std::move(candidate));
            }
        }
        else if constexpr (is_std_pair_v<U>) {
            assignContainerValueFrom(dest.first, source.first);
            assignContainerValueFrom(dest.second, source.second);
        }
        else if constexpr (is_std_tuple_v<U>) {
            assignTupleValueFrom(dest, source, std::make_index_sequence<std::tuple_size_v<U>>{});
        }
        else {
            static_assert(std::is_assignable_v<T&, Source&>);
            dest = source;
        }
    }

    template <class Dest, class Source>
    void assignAdaptedContainerValue(Dest& dest, Source& source) {
        using DestValue = std::remove_reference_t<Dest>;
        using SourceValue = std::remove_reference_t<Source>;
        if constexpr (std::is_array_v<DestValue> && is_std_array_v<SourceValue>) {
            static_assert(std::extent_v<DestValue> == std::tuple_size_v<SourceValue>);
            for (std::size_t i = 0; i < std::extent_v<DestValue>; ++i) {
                assignAdaptedContainerValue(dest[i], source[i]);
            }
        }
        else {
            assignContainerValueFrom(dest, source);
        }
    }

} // namespace luax::detail

namespace luax {
    template <class T>
    void assignContainerValue(T& dest, T const& source) {
        if (&dest == &source) {
            return;
        }
        detail::assignContainerValueFrom(dest, source);
    }

    template <class T>
    void assignContainerValue(T& dest, T&& source) {
        if (&dest == &source) {
            return;
        }
        detail::assignContainerValueFrom(dest, source);
    }

    template <class T, std::size_t N, class U>
    void assignContainerValue(U (&dest)[N], std::array<T, N> source) {
        detail::assignAdaptedContainerValue(dest, source);
    }

    template <class T>
    void checkContainerValue(lua_State* L, int idx, char const* label, T& out) {
        detail::checkContainerValueInto(L, idx, label, out);
    }

    template <class T>
    T checkContainerValue(lua_State* L, int idx, char const* label) {
        T result{};
        checkContainerValue(L, idx, label, result);
        return T(result);
    }

} // namespace luax
