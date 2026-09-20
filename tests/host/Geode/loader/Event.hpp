#pragma once

#include <Geode/utils/web.hpp>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace geode::detail {
    inline std::vector<std::function<void()>>& eventResetters() {
        static std::vector<std::function<void()>> resetters;
        return resetters;
    }
} // namespace geode::detail

namespace geode {
    template <class Marker, class Signature, class... Filter>
    struct Event {
        using FilterKey = std::tuple<Filter...>;

        struct Entry {
            int priority = 0;
            std::shared_ptr<std::function<Signature>> callback;
            bool active = true;
        };

        static std::map<FilterKey, std::vector<Entry>>& storage() {
            static thread_local std::map<FilterKey, std::vector<Entry>> entries;
            static thread_local bool registered = [] {
                geode::detail::eventResetters().emplace_back([] {
                    entries.clear();
                });
                return true;
            }();
            (void)registered;
            return entries;
        }

        Event() = default;

        template <class... FArgs>
            requires(sizeof...(FArgs) == sizeof...(Filter) && sizeof...(FArgs) != 0)
        explicit Event(FArgs... value) : m_filter(std::make_tuple(std::move(value)...)) {}

        template <class Callable>
        ListenerHandle listen(Callable callable, int priority = 0) const {
            auto callback = std::make_shared<std::function<Signature>>(
                [callable = std::move(callable)](auto&&... args) {
                    using Result = std::invoke_result_t<Callable, decltype(args)...>;
                    if constexpr (std::is_convertible_v<Result, bool>) {
                        return static_cast<bool>(
                            std::invoke(callable, std::forward<decltype(args)>(args)...)
                        );
                    }
                    else {
                        std::invoke(callable, std::forward<decltype(args)>(args)...);
                        return false;
                    }
                }
            );
            storage()[m_filter].push_back({priority, callback, true});
            return ListenerHandle([callback, filter = m_filter]() {
                auto& entries = storage()[filter];
                for (auto& entry : entries) {
                    if (entry.callback == callback) {
                        entry.active = false;
                    }
                }
            });
        }

        std::size_t getReceiverCount() const {
            auto const it = storage().find(m_filter);
            return it == storage().end() ? 0 : it->second.size();
        }

        template <class... Args>
        bool send(Args&&... args) const {
            auto const it = storage().find(m_filter);
            if (it == storage().end()) {
                return false;
            }
            auto entries = it->second;
            std::stable_sort(entries.begin(), entries.end(), [](Entry const& a, Entry const& b) {
                return a.priority < b.priority;
            });
            for (auto const& entry : entries) {
                if (!entry.active || !entry.callback) {
                    continue;
                }
                if (std::invoke(*entry.callback, std::forward<Args>(args)...)) {
                    return true;
                }
            }
            return false;
        }

    private:
        FilterKey m_filter{};
    };

    namespace test {
        inline void resetEvents() {
            auto resetters = geode::detail::eventResetters();
            for (auto& reset : resetters) {
                reset();
            }
        }
    } // namespace test
} // namespace geode
