#include "lua/bindings/Binding.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <fmt/format.h>
#include <vector>

namespace luax {
    namespace {
        std::vector<Binding>& bindings() {
            static auto* value = new std::vector<Binding>;
            return *value;
        }
    } // namespace

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

#if defined(LUAUAPI_HOST_TESTS)
    void resetBindingsForTests() {
        bindings().clear();
    }
#endif
} // namespace luax
