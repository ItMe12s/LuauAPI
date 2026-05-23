#include "Binding.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <exception>
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

    void applyAllBindings(lua_State* L) {
        auto ordered = bindings();
        std::stable_sort(ordered.begin(), ordered.end(), [](auto const& left, auto const& right) {
            return left.priority < right.priority;
        });

        for (auto const& binding : ordered) {
            try {
                binding.fn(L);
            } catch (std::exception const& e) {
                geode::log::error("[lua:bind:{}] {}", binding.name, e.what());
            } catch (...) {
                geode::log::error("[lua:bind:{}] unknown exception", binding.name);
            }
        }
    }
}
