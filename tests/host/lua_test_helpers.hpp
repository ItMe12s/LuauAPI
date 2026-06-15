#pragma once

#include "core/Runtime.hpp"
#include "framework/Binding.hpp"
#include "framework/usertype/LuaRef.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <lualib.h>

#include <memory>
#include <string>
#include <string_view>

namespace luauapi_test {
    struct LuaStateDeleter {
        void operator()(lua_State* L) const {
            if (L) {
                lua_close(L);
            }
        }
    };

    using LuaStatePtr = std::unique_ptr<lua_State, LuaStateDeleter>;

    inline LuaStatePtr makeLuaState(bool openLibs = false) {
        auto* L = luaL_newstate();
        REQUIRE(L != nullptr);
        if (openLibs) {
            luaL_openlibs(L);
        }
        return LuaStatePtr(L);
    }

    inline std::string compile(std::string_view source) {
        return luax::Runtime::compileSource(source);
    }

    struct BindingGuard {
        BindingGuard() {
            luax::resetBindingsForTests();
        }

        ~BindingGuard() {
            luax::resetBindingsForTests();
        }
    };

    inline void loadFunction(lua_State* L, std::string_view source, char const* chunk = "=test") {
        auto bytecode = compile(source);
        REQUIRE(luau_load(L, chunk, bytecode.data(), bytecode.size(), 0) == 0);
    }

    inline luax::LuaRef makeCallback(lua_State* L, std::string_view source) {
        loadFunction(L, source);
        luax::LuaRef ref(L, -1);
        lua_pop(L, 1);
        return ref;
    }
} // namespace luauapi_test
