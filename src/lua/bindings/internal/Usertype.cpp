#include "Usertype.hpp"

#include <Geode/Geode.hpp>
#include <lua.h>
#include <luaconf.h>
#include <lualib.h>

#include <new>

namespace luax::detail {
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
        }
        block->~UserdataBlock();
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

    geode::Result<void> ensureUserdataMetatable(lua_State* L, TypeInfo const& info) {
        if (info.tag == 0 || info.tag >= LUA_UTAG_LIMIT) {
            return geode::Err(fmt::format("{} userdata tag {} exceeds LUA_UTAG_LIMIT ({})", info.name, info.tag, LUA_UTAG_LIMIT));
        }

        lua_getuserdatametatable(L, static_cast<int>(info.tag));
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return geode::Ok();
        }
        lua_pop(L, 1);

        luaL_getmetatable(L, info.mtName.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return geode::Err(fmt::format("missing userdata metatable for {}", info.name));
        }
        lua_setuserdatametatable(L, static_cast<int>(info.tag));
        return geode::Ok();
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

    void pushUserdataOwned(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info) {
        auto* storage = lua_newuserdatataggedwithmetatable(L, sizeof(UserdataBlock), static_cast<int>(info.tag));
        auto* block = new (storage) UserdataBlock();
        block->ptr = obj;
        block->flags = 1u;
    }

    void pushUserdataBorrowed(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info) {
        auto* storage = lua_newuserdatataggedwithmetatable(L, sizeof(UserdataBlock), static_cast<int>(info.tag));
        auto* block = new (storage) UserdataBlock();
        block->ptr = nullptr;
        block->weak = geode::WeakRef<cocos2d::CCObject>(obj);
        block->flags = 0u;
    }
}
