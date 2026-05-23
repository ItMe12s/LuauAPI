#include "Usertype.hpp"

#include <lua.h>
#include <lualib.h>

namespace luax::detail {
    UsertypeRegistry& UsertypeRegistry::get() {
        static UsertypeRegistry instance;
        return instance;
    }

    std::uint32_t UsertypeRegistry::tagFor(std::type_index idx) {
        auto it = m_byType.find(idx);
        if (it != m_byType.end()) return it->second.tag;
        TypeInfo info;
        info.tag = m_next++;
        auto inserted = m_byType.emplace(idx, std::move(info));
        m_byTag[inserted.first->second.tag] = &inserted.first->second;
        return inserted.first->second.tag;
    }

    TypeInfo& UsertypeRegistry::infoFor(std::type_index idx) {
        auto it = m_byType.find(idx);
        if (it != m_byType.end()) return it->second;
        TypeInfo info;
        info.tag = m_next++;
        auto inserted = m_byType.emplace(idx, std::move(info));
        m_byTag[inserted.first->second.tag] = &inserted.first->second;
        return inserted.first->second;
    }

    TypeInfo const* UsertypeRegistry::findByTag(std::uint32_t tag) const {
        auto it = m_byTag.find(tag);
        return it == m_byTag.end() ? nullptr : it->second;
    }

    void destructorDispatch(lua_State*, void* ud) {
        auto* block = static_cast<UserdataBlock*>(ud);
        if (!block || !block->ptr) return;
        if (block->flags & 1u) {
            releaseLuaRetain(block->ptr, "__gc", false);
        }
        block->ptr = nullptr;
    }

    void getOrCreateMetatable(lua_State* L, TypeInfo& info) {
        if (luaL_newmetatable(L, info.mtName.c_str())) {
            lua_createtable(L, 0, 16);
            lua_setfield(L, -2, "__index");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            lua_pushstring(L, info.name.c_str());
            lua_setfield(L, -2, "__type");
        }
        lua_pop(L, 1);
    }

    void chainMethodTable(lua_State* L, TypeInfo const& info, std::uint32_t baseTag) {
        auto const* base = UsertypeRegistry::get().findByTag(baseTag);
        if (!base) return;
        // Gives T's method table a metatable that falls back to the base's method table.
        // Missing keys on a derived instance then walk up to inherited methods.
        luaL_getmetatable(L, info.mtName.c_str());
        lua_getfield(L, -1, "__index");
        luaL_getmetatable(L, base->mtName.c_str());
        lua_getfield(L, -1, "__index");
        lua_remove(L, -2);
        lua_createtable(L, 0, 1);
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -3);
        lua_pop(L, 3);
    }

    void appendMethod(lua_State* L, TypeInfo const& info, char const* name, lua_CFunction fn) {
        luaL_getmetatable(L, info.mtName.c_str());
        lua_getfield(L, -1, "__index");
        lua_pushcfunction(L, fn, name);
        lua_setfield(L, -2, name);
        lua_pop(L, 2);
    }

    cocos2d::CCObject* checkUserdata(lua_State* L, int idx, std::uint32_t targetTag, char const* targetName, char const* method) {
        if (lua_type(L, idx) != LUA_TUSERDATA) {
            luaL_error(L, "%s expected %s at arg %d", method, targetName, idx);
        }
        int tagInt = lua_userdatatag(L, idx);
        if (tagInt <= 0) {
            luaL_error(L, "%s expected %s at arg %d", method, targetName, idx);
        }
        auto* info = UsertypeRegistry::get().findByTag(static_cast<std::uint32_t>(tagInt));
        if (!info) {
            luaL_error(L, "%s expected %s at arg %d", method, targetName, idx);
        }
        bool ok = false;
        for (auto b : info->baseClosure) {
            if (b == targetTag) { ok = true; break; }
        }
        if (!ok) {
            luaL_error(L, "%s expected %s at arg %d", method, targetName, idx);
        }
        auto* block = static_cast<UserdataBlock*>(lua_touserdata(L, idx));
        if (!block || !block->ptr) {
            luaL_error(L, "%s expected live %s at arg %d", method, targetName, idx);
        }
        return block->ptr;
    }

    void pushUserdata(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info, bool owned) {
        auto* block = static_cast<UserdataBlock*>(
            lua_newuserdatatagged(L, sizeof(UserdataBlock), static_cast<int>(info.tag)));
        block->ptr = obj;
        block->flags = owned ? 1u : 0u;
        luaL_getmetatable(L, info.mtName.c_str());
        lua_setmetatable(L, -2);
    }
}
