#pragma once

// Defines the gd:: aliases in both the real SDK and the host-test stub,
// so this header is self-contained and no include-order contract is needed.
#include <Geode/utils/cocos.hpp>
#include <array>
#include <tuple>
#include <type_traits>

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
} // namespace luax::detail
