#pragma once

#include <Geode/Result.hpp>
#include <cstdint>
#include <lua.h>

namespace luax {
    struct OpaqueHandleBlock {
        void* ptr = nullptr;
    };

    namespace detail {
        constexpr int opaqueHandleTag() noexcept {
            return 1;
        }

        bool isOpaqueHandle(lua_State* L, int idx);
        void ensureOpaqueHandleMetatable(lua_State* L);
    } // namespace detail

    geode::Result<void> registerOpaqueHandle(lua_State* L);

    void pushOpaqueHandle(lua_State* L, void* ptr);

    void* checkOpaqueHandle(lua_State* L, int idx, char const* label);

    void* tryOpaqueHandle(lua_State* L, int idx);

    template <class T>
    T* checkOpaqueHandle(lua_State* L, int idx, char const* label) {
        return static_cast<T*>(checkOpaqueHandle(L, idx, label));
    }

    template <class T>
    void pushOpaqueHandle(lua_State* L, T* ptr) {
        pushOpaqueHandle(L, static_cast<void*>(ptr));
    }
} // namespace luax
