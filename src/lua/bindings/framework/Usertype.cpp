#include "lua/bindings/framework/Usertype.hpp"

#include "lua/Config.hpp"
#include "lua/bindings/framework/BindingHost.hpp"
#include "lua/bindings/framework/Fields.hpp"
#include "lua/bindings/framework/OpaqueHandle.hpp"

#include <Geode/Geode.hpp>
#include <cstring>
#include <lua.h>
#include <luaconf.h>
#include <lualib.h>
#include <new>

namespace luax::detail {
    namespace {
        bool invokeFieldAccessor(
            BindingHost* host, int nargs, int nresults, std::string_view context, int deadlineMs
        ) {
            if (!host) {
                return false;
            }
            if (host->ready()) {
                return host->protectedCall(nargs, nresults, context, deadlineMs).isOk();
            }
            return host->protectedCallWithTraceback(nargs, nresults, context).isOk();
        }

        int usertypeIndex(lua_State* L) {
            if (!lua_getmetatable(L, 1)) {
                lua_pushnil(L);
                return 1;
            }
            lua_getfield(L, -1, "__methods");
            lua_pushvalue(L, 2);
            lua_gettable(L, -2);
            if (!lua_isnil(L, -1)) {
                return 1;
            }
            lua_pop(L, 2);

            lua_getfield(L, -1, "__fields");
            lua_pushvalue(L, 2);
            lua_gettable(L, -2);
            if (lua_istable(L, -1)) {
                int top = lua_gettop(L);
                lua_getfield(L, -1, "get");
                if (lua_isfunction(L, -1)) {
                    lua_pushvalue(L, 1);
                    if (auto* host = BindingHost::getIfInitialized()) {
                        if (invokeFieldAccessor(
                                host, 1, 1, "usertype field get", kHookScriptDeadlineMs
                            )) {
                            return 1;
                        }
                        lua_settop(L, top);
                        luaL_error(L, "usertype field get failed");
                    }
                    lua_settop(L, top);
                    luaL_error(L, "luau runtime not available");
                }
                lua_settop(L, top);
            }
            lua_pop(L, 2);

            auto* key = lua_tostring(L, 2);
            auto* node = tryNodeCandidate(L, 1);
            if (node && key && std::strcmp(key, "m_fields") == 0) {
                Fields::push(L, node);
                return 1;
            }
            if (node) {
                if (Fields::tryPush(L, node)) {
                    lua_pushvalue(L, 2);
                    lua_gettable(L, -2);
                }
                else {
                    lua_pushnil(L);
                }
                return 1;
            }
            lua_pushnil(L);
            return 1;
        }

        int usertypeNewIndex(lua_State* L) {
            if (!lua_getmetatable(L, 1)) {
                luaL_error(L, "userdata has no metatable");
                return 0;
            }
            lua_getfield(L, -1, "__fields");
            lua_pushvalue(L, 2);
            lua_gettable(L, -2);
            if (lua_istable(L, -1)) {
                int top = lua_gettop(L);
                lua_getfield(L, -1, "set");
                if (lua_isfunction(L, -1)) {
                    lua_pushvalue(L, 1);
                    lua_pushvalue(L, 3);
                    if (auto* host = BindingHost::getIfInitialized()) {
                        if (!invokeFieldAccessor(
                                host, 2, 0, "usertype field set", kHookScriptDeadlineMs
                            )) {
                            lua_settop(L, top);
                            luaL_error(L, "usertype field set failed");
                        }
                        return 0;
                    }
                    lua_settop(L, top);
                    luaL_error(L, "luau runtime not available");
                }
                lua_settop(L, top);
            }
            lua_pop(L, 3);

            auto* key = lua_tostring(L, 2);
            if (key && std::strcmp(key, "m_fields") == 0) {
                luaL_error(L, "m_fields is read-only");
                return 0;
            }
            auto* node = tryNodeCandidate(L, 1);
            if (!node) {
                luaL_error(L, "unknown field on non-CCNode userdata");
                return 0;
            }
            Fields::push(L, node);
            lua_pushvalue(L, 2);
            lua_pushvalue(L, 3);
            lua_settable(L, -3);
            return 0;
        }

        int readOnlyFieldSetter(lua_State* L) {
            auto* field = lua_tostring(L, lua_upvalueindex(1));
            luaL_error(L, "%s is read-only", field ? field : "field");
            return 0;
        }
    } // namespace

    cocos2d::CCObject* liveObject(UserdataBlock* block) {
        if (!block) return nullptr;
        if (block->flags & kUserdataOwnedFlag) {
            return block->ptr;
        }
        return block->weak.lock().data();
    }

    void destructorDispatch(lua_State*, void* ud) {
        auto* block = static_cast<UserdataBlock*>(ud);
        if (!block) return;
        if (block->flags & kUserdataOwnedFlag) {
            if (block->ptr) {
                releaseLuaRetain(block->ptr, "__gc", false);
            }
        }
        block->~UserdataBlock();
    }

    void getOrCreateMetatable(lua_State* L, TypeInfo& info) {
        if (luaL_newmetatable(L, info.mtName.c_str())) {
            lua_createtable(L, 0, 16);
            lua_setfield(L, -2, "__methods");
            lua_createtable(L, 0, 16);
            lua_setfield(L, -2, "__fields");
            lua_pushcfunction(L, &usertypeIndex, "__index");
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, &usertypeNewIndex, "__newindex");
            lua_setfield(L, -2, "__newindex");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
            lua_pushstring(L, info.name.c_str());
            lua_setfield(L, -2, "__type");
        }
        lua_pop(L, 1);
    }

    geode::Result<void> ensureUserdataMetatable(lua_State* L, TypeInfo const& info) {
        if (!isValidUserdataTag(info.tag)) {
            return geode::Err(fmt::format("{} invalid userdata tag {}", info.name, info.tag));
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
        lua_getfield(L, -1, "__methods");
        luaL_getmetatable(L, base->mtName.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, 3);
            return;
        }
        lua_getfield(L, -1, "__methods");
        lua_remove(L, -2);
        lua_createtable(L, 0, 1);
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -3);
        lua_pop(L, 2);

        lua_getfield(L, -1, "__fields");
        luaL_getmetatable(L, base->mtName.c_str());
        if (lua_isnil(L, -1)) {
            lua_pop(L, 3);
            return;
        }
        lua_getfield(L, -1, "__fields");
        lua_remove(L, -2);
        lua_createtable(L, 0, 1);
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -3);
        lua_pop(L, 3);
    }

    void appendMethod(lua_State* L, TypeInfo const& info, char const* name, lua_CFunction fn) {
        luaL_getmetatable(L, info.mtName.c_str());
        lua_getfield(L, -1, "__methods");
        lua_pushcfunction(L, fn, name);
        lua_setfield(L, -2, name);
        lua_pop(L, 2);
    }

    void appendField(
        lua_State* L, TypeInfo const& info, char const* name, lua_CFunction getter, lua_CFunction setter
    ) {
        luaL_getmetatable(L, info.mtName.c_str());
        lua_getfield(L, -1, "__fields");
        lua_createtable(L, 0, 2);
        lua_pushcfunction(L, getter, name);
        lua_setfield(L, -2, "get");
        lua_pushcfunction(L, setter, name);
        lua_setfield(L, -2, "set");
        lua_setfield(L, -2, name);
        lua_pop(L, 2);
    }

    void appendReadOnlyField(lua_State* L, TypeInfo const& info, char const* name, lua_CFunction getter) {
        luaL_getmetatable(L, info.mtName.c_str());
        lua_getfield(L, -1, "__fields");
        lua_createtable(L, 0, 2);
        lua_pushcfunction(L, getter, name);
        lua_setfield(L, -2, "get");
        lua_pushstring(L, name);
        lua_pushcclosure(L, &readOnlyFieldSetter, "readonlyFieldSetter", 1);
        lua_setfield(L, -2, "set");
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
        if (detail::isReservedUserdataTag(static_cast<std::uint32_t>(tagInt))) {
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
        return {obj, info};
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
        return {obj, info};
    }

    cocos2d::CCNode* tryNodeCandidate(lua_State* L, int idx) {
        auto candidate = tryCandidate(L, idx);
        if (!candidate.obj || !candidate.info) return nullptr;
        if (auto* typed = geode::cast::typeinfo_cast<cocos2d::CCNode*>(candidate.obj)) {
            return typed;
        }
        auto const* nodeInfo =
            UsertypeRegistry::get().findInfo(std::type_index(typeid(cocos2d::CCNode)));
        if (!nodeInfo) return nullptr;
        if (isValidUserdataTag(nodeInfo->tag) && hasBase(*candidate.info, nodeInfo->tag)) {
            return static_cast<cocos2d::CCNode*>(candidate.obj);
        }
        return nullptr;
    }

    void pushUserdataOwned(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info) {
        if (!isValidUserdataTag(info.tag)) {
            lua_pushnil(L);
            return;
        }
        auto* storage =
            lua_newuserdatataggedwithmetatable(L, sizeof(UserdataBlock), static_cast<int>(info.tag));
        auto* block = new (storage) UserdataBlock();
        block->ptr = obj;
        block->flags = kUserdataOwnedFlag;
    }

    void pushUserdataBorrowed(lua_State* L, cocos2d::CCObject* obj, TypeInfo const& info) {
        if (!isValidUserdataTag(info.tag)) {
            lua_pushnil(L);
            return;
        }
        auto* storage =
            lua_newuserdatataggedwithmetatable(L, sizeof(UserdataBlock), static_cast<int>(info.tag));
        auto* block = new (storage) UserdataBlock();
        block->ptr = nullptr;
        block->weak = geode::WeakRef<cocos2d::CCObject>(obj);
        block->flags = 0u;
    }
} // namespace luax::detail
