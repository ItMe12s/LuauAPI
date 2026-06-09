#pragma once

#include "ReadOnlyView.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <cstddef>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace luax::detail {
    template <class T, class BackingPolicy, class ElementPolicy>
    char const* readOnlySequenceViewMetatable() {
        static auto const name = std::string(ElementPolicy::kPrefix) + typeid(T).name() + ">";
        return name.c_str();
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    typename BackingPolicy::Block* checkReadOnlySequenceView(lua_State* L, int idx) {
        auto* block = static_cast<typename BackingPolicy::Block*>(
            luaL_checkudata(L, idx, readOnlySequenceViewMetatable<T, BackingPolicy, ElementPolicy>())
        );
        if (!block || !BackingPolicy::storageLive(*block)) {
            luaL_error(L, BackingPolicy::kLiveError);
        }
        if (!BackingPolicy::ownerLive(*block)) {
            luaL_error(L, BackingPolicy::kLiveError);
        }
        return block;
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    int readOnlySequenceViewIndex(lua_State* L) {
        auto* block = checkReadOnlySequenceView<T, BackingPolicy, ElementPolicy>(L, 1);
        auto index = luaL_checkinteger(L, 2);
        auto const size = BackingPolicy::size(*block);
        if (index < 1 || static_cast<std::size_t>(index) > size) {
            lua_pushnil(L);
            return 1;
        }
        ElementPolicy::template pushElement<T>(
            L, BackingPolicy::at(*block, static_cast<std::size_t>(index - 1))
        );
        return 1;
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    int readOnlySequenceViewLen(lua_State* L) {
        auto* block = checkReadOnlySequenceView<T, BackingPolicy, ElementPolicy>(L, 1);
        lua_pushinteger(L, static_cast<lua_Integer>(BackingPolicy::size(*block)));
        return 1;
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    int readOnlySequenceViewNewIndex(lua_State* L) {
        luaL_error(L, BackingPolicy::kNewIndexError);
        return 0;
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    int readOnlySequenceViewGc(lua_State* L) {
        auto* block = static_cast<typename BackingPolicy::Block*>(
            luaL_checkudata(L, 1, readOnlySequenceViewMetatable<T, BackingPolicy, ElementPolicy>())
        );
        if (block) {
            using Block = typename BackingPolicy::Block;
            block->~Block();
        }
        return 0;
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    void ensureReadOnlySequenceViewMetatable(lua_State* L) {
        registerReadOnlyMetatable(
            L,
            readOnlySequenceViewMetatable<T, BackingPolicy, ElementPolicy>(),
            &readOnlySequenceViewIndex<T, BackingPolicy, ElementPolicy>,
            &readOnlySequenceViewLen<T, BackingPolicy, ElementPolicy>,
            &readOnlySequenceViewNewIndex<T, BackingPolicy, ElementPolicy>,
            &readOnlySequenceViewGc<T, BackingPolicy, ElementPolicy>,
            BackingPolicy::kTypeName
        );
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    std::enable_if_t<BackingPolicy::kSupportsCopy, bool> tryCopyReadOnlySequenceView(
        lua_State* L, int idx, typename BackingPolicy::CopyOut& out
    ) {
        if (!lua_isuserdata(L, idx)) {
            return false;
        }
        using Block = typename BackingPolicy::Block;
        auto* block = static_cast<Block*>(nullptr);
        if (lua_getmetatable(L, idx)) {
            luaL_getmetatable(L, (readOnlySequenceViewMetatable<T, BackingPolicy, ElementPolicy>()));
            if (lua_rawequal(L, -1, -2)) {
                block = static_cast<Block*>(lua_touserdata(L, idx));
            }
            lua_pop(L, 2);
        }
        if (!block || !BackingPolicy::storageLive(*block)) {
            return false;
        }
        if (!BackingPolicy::ownerLive(*block)) {
            return false;
        }
        BackingPolicy::copyOut(*block, out);
        return true;
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    std::enable_if_t<BackingPolicy::kSupportsBorrowed, void> pushBorrowedSequenceView(
        lua_State* L, typename BackingPolicy::BorrowedArgs const& args
    ) {
        if (!BackingPolicy::canPushBorrowed(args)) {
            lua_pushnil(L);
            return;
        }
        ensureReadOnlySequenceViewMetatable<T, BackingPolicy, ElementPolicy>(L);
        using Block = typename BackingPolicy::Block;
        auto* storage = lua_newuserdata(L, sizeof(Block));
        auto* block = new (storage) Block();
        BackingPolicy::initBorrowed(*block, args);
        luaL_getmetatable(L, (readOnlySequenceViewMetatable<T, BackingPolicy, ElementPolicy>()));
        lua_setmetatable(L, -2);
    }

    template <class T, class BackingPolicy, class ElementPolicy>
    std::enable_if_t<BackingPolicy::kSupportsOwned, void> pushOwnedSequenceView(
        lua_State* L, typename BackingPolicy::OwnedArgs const& args
    ) {
        ensureReadOnlySequenceViewMetatable<T, BackingPolicy, ElementPolicy>(L);
        using Block = typename BackingPolicy::Block;
        auto* storage = lua_newuserdata(L, sizeof(Block));
        auto* block = new (storage) Block();
        BackingPolicy::initOwned(*block, args);
        luaL_getmetatable(L, (readOnlySequenceViewMetatable<T, BackingPolicy, ElementPolicy>()));
        lua_setmetatable(L, -2);
    }
} // namespace luax::detail
