#include "Binding.hpp"

#include <Geode/Geode.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <vector>

namespace luax {
    namespace {
        std::vector<Binding>& bindings() {
            static std::vector<Binding> value;
            return value;
        }
    }

    void registerBinding(Binding const& binding) {
        bindings().push_back(binding);
    }

    std::optional<std::string> applyAllBindings(lua_State* L) {
        auto ordered = bindings();
        std::stable_sort(ordered.begin(), ordered.end(), [](auto const& left, auto const& right) {
            return left.priority < right.priority;
        });

        for (auto const& binding : ordered) {
            auto result = binding.fn(L);
            if (result.isErr()) {
                auto msg = fmt::format("[lua:bind:{}] {}", binding.name, result.unwrapErr());
                geode::log::error("{}", msg);
                return msg;
            }
        }
        return std::nullopt;
    }
}
