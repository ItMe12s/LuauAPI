#pragma once

#include "framework/stack/ContainerTraits.hpp"

#include <lua.h>
#include <type_traits>

namespace luax {
    template <class T>
    T checkContainerValue(lua_State* L, int idx, char const* label);

    template <class T>
    void pushContainerValue(lua_State* L, T const& value, cocos2d::CCObject* owner);

    template <class T>
    void assignContainerValue(T& dest, T const& source);

    template <class T>
    void assignContainerValue(T& dest, T&& source);
} // namespace luax

namespace luax::detail {
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
