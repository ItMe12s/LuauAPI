#include "Usertype.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <luaconf.h>
#include <lualib.h>

namespace luax::detail {
    UsertypeRegistry& UsertypeRegistry::get() {
        static auto* instance = new UsertypeRegistry;
        return *instance;
    }

    std::uint32_t UsertypeRegistry::tagFor(std::type_index idx) {
        auto it = m_byType.find(idx);
        if (it != m_byType.end()) return it->second.tag;
        TypeInfo info;
        info.tag = m_next++;
        if (info.tag >= LUA_UTAG_LIMIT) {
            geode::log::error("UsertypeRegistry: tag {} exceeds LUA_UTAG_LIMIT ({})", info.tag, LUA_UTAG_LIMIT);
        }
        auto inserted = m_byType.emplace(idx, std::move(info));
        m_byTag[inserted.first->second.tag] = &inserted.first->second;
        return inserted.first->second.tag;
    }

    TypeInfo& UsertypeRegistry::infoFor(std::type_index idx) {
        auto it = m_byType.find(idx);
        if (it != m_byType.end()) return it->second;
        TypeInfo info;
        info.tag = m_next++;
        if (info.tag >= LUA_UTAG_LIMIT) {
            geode::log::error("UsertypeRegistry: tag {} exceeds LUA_UTAG_LIMIT ({})", info.tag, LUA_UTAG_LIMIT);
        }
        auto inserted = m_byType.emplace(idx, std::move(info));
        m_byTag[inserted.first->second.tag] = &inserted.first->second;
        return inserted.first->second;
    }

    TypeInfo const* UsertypeRegistry::findByTag(std::uint32_t tag) const {
        auto it = m_byTag.find(tag);
        return it == m_byTag.end() ? nullptr : it->second;
    }

    cocos2d::CCObject* liveObject(UserdataBlock* block) {
        if (!block) return nullptr;
        if (block->flags & 1u) {
            return block->ptr;
        }
        return block->weak.lock().data();
    }

    void destructorDispatch(lua_State*, void* ud) {
        auto* block = static_cast<UserdataBlock*>(ud);
        if (!block) return;
        if (block->flags & 1u) {
            if (block->ptr) {
                releaseLuaRetain(block->ptr, "__gc", false);
            }
            block->ptr = nullptr;
        }
        block->weak = geode::WeakRef<cocos2d::CCObject>{};
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
        if (!base || base->mtName.empty()) return;
        luaL_getmetatable(L, info.mtName.c_str());
        lua_getfield(L, -1, "__index");
        luaL_getmetatable(L, base->mtName.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, 3);
            return;
        }
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

    bool hasBase(TypeInfo const& info, std::uint32_t targetTag) {
        for (auto b : info.baseClosure) {
            if (b == targetTag) return true;
        }
        return false;
    }

    UserdataCandidate checkCandidate(lua_State* L, int idx, char const* targetName, char const* method) {
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
        auto* block = static_cast<UserdataBlock*>(lua_touserdata(L, idx));
        auto* obj = liveObject(block);
        if (!obj) {
            luaL_error(L, "%s expected live %s at arg %d", method, targetName, idx);
        }
        return { obj, info };
    }

    UserdataCandidate tryCandidate(lua_State* L, int idx) {
        if (lua_type(L, idx) != LUA_TUSERDATA) return {};
        int tagInt = lua_userdatatag(L, idx);
        if (tagInt <= 0) return {};
        auto* info = UsertypeRegistry::get().findByTag(static_cast<std::uint32_t>(tagInt));
        if (!info) return {};
        auto* block = static_cast<UserdataBlock*>(lua_touserdata(L, idx));
        auto* obj = liveObject(block);
        if (!obj) return {};
        return { obj, info };
    }

    cocos2d::CCObject* checkUserdata(lua_State* L, int idx, std::uint32_t targetTag, char const* targetName, char const* method) {
        auto candidate = checkCandidate(L, idx, targetName, method);
        if (!hasBase(*candidate.info, targetTag)) {
            luaL_error(L, "%s expected %s at arg %d", method, targetName, idx);
        }
        return candidate.obj;
    }

    void pushUserdataOwned(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info) {
        auto* block = static_cast<UserdataBlock*>(
            lua_newuserdatatagged(L, sizeof(UserdataBlock), static_cast<int>(info.tag)));
        block->ptr = obj;
        block->weak = geode::WeakRef<cocos2d::CCObject>{};
        block->flags = 1u;
        luaL_getmetatable(L, info.mtName.c_str());
        lua_setmetatable(L, -2);
    }

    void pushUserdataBorrowed(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info) {
        auto* block = static_cast<UserdataBlock*>(
            lua_newuserdatatagged(L, sizeof(UserdataBlock), static_cast<int>(info.tag)));
        block->ptr = nullptr;
        block->weak = geode::WeakRef<cocos2d::CCObject>(obj);
        block->flags = 0u;
        luaL_getmetatable(L, info.mtName.c_str());
        lua_setmetatable(L, -2);
    }
}
