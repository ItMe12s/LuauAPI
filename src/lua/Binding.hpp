#pragma once

#include <lua.h>

namespace luax {
    using Registrar = void(*)(lua_State*);

    struct Binding {
        char const* name;
        Registrar fn;
        int priority;
    };

    void registerBinding(Binding const& binding);
    void applyAllBindings(lua_State* L);
}

#define LUAX_BINDING_PRIORITY(NAME, FN, PRIO)                \
    namespace {                                              \
        struct AutoReg_##NAME {                              \
            AutoReg_##NAME() {                               \
                ::luax::registerBinding({#NAME, &FN, PRIO}); \
            }                                                \
        } _autoreg_##NAME;                                   \
    }

#define LUAX_BINDING(NAME, FN) LUAX_BINDING_PRIORITY(NAME, FN, 10)
