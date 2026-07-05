#pragma once

#include <algorithm>
#include <concepts>
#include <optional>
#include <vector>

namespace geode::utils::ranges {
    template <class C>
    concept ValidConstContainer = requires(C const& c) {
        c.begin();
        c.end();
        { c.size() } -> std::convertible_to<std::size_t>;
        typename C::value_type;
    };

    template <class C>
    concept ValidMutContainer = requires(C& c) {
        c.begin();
        c.end();
        { c.size() } -> std::convertible_to<std::size_t>;
        typename C::value_type;
    };

    template <ValidConstContainer C>
    bool contains(C const& cont, typename C::value_type const& elem) {
        return std::find(cont.begin(), cont.end(), elem) != cont.end();
    }

    template <ValidConstContainer C, class Predicate>
    bool contains(C const& cont, Predicate fun) {
        return std::find_if(cont.begin(), cont.end(), fun) != cont.end();
    }

    template <ValidConstContainer C, class Predicate>
    std::optional<typename C::value_type> find(C const& cont, Predicate fun) {
        auto it = std::find_if(cont.begin(), cont.end(), fun);
        if (it != cont.end()) {
            return *it;
        }
        return std::nullopt;
    }

    template <ValidConstContainer C>
    std::optional<std::size_t> indexOf(C const& cont, typename C::value_type const& elem) {
        auto it = std::find(cont.begin(), cont.end(), elem);
        if (it != cont.end()) {
            return static_cast<std::size_t>(std::distance(cont.begin(), it));
        }
        return std::nullopt;
    }

    template <ValidMutContainer C, class Predicate>
    C& remove(C& container, Predicate fun) {
        container.erase(std::remove_if(container.begin(), container.end(), fun), container.end());
        return container;
    }
} // namespace geode::utils::ranges
