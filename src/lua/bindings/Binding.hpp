#pragma once

#include <Geode/Geode.hpp>
#include <lua.h>
#include <optional>
#include <string>

namespace luax {
    using Registrar = geode::Result<void> (*)(lua_State*);

    struct Binding {
        char const* name;
        Registrar fn;
        int priority;
    };

    void registerBinding(Binding const& binding);
    std::optional<std::string> applyAllBindings(lua_State* L);

#if defined(LUAUAPI_HOST_TESTS)
    void resetBindingsForTests();
#endif
} // namespace luax

#define LUAX_BINDING_PRIORITY(NAME, FN, PRIO)                \
    namespace {                                              \
        struct AutoReg_##NAME {                              \
            AutoReg_##NAME() {                               \
                ::luax::registerBinding({#NAME, &FN, PRIO}); \
            }                                                \
        } _autoreg_##NAME;                                   \
    }

#define LUAX_BINDING(NAME, FN) LUAX_BINDING_PRIORITY(NAME, FN, 10)
