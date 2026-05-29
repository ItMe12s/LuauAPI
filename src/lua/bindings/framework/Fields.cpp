#include "Fields.hpp"

#include <Geode/Geode.hpp>

#include <unordered_map>

namespace luax {
    namespace {
        std::unordered_map<cocos2d::CCNode*, LuaRef>& fieldTables() {
            static auto* value = new std::unordered_map<cocos2d::CCNode*, LuaRef>();
            return *value;
        }
    }

    void Fields::push(lua_State* L, cocos2d::CCNode* node) {
        if (!L) return;
        if (!node) {
            lua_pushnil(L);
            return;
        }
        auto& tables = fieldTables();
        auto it = tables.find(node);
        if (it != tables.end() && it->second.push()) {
            return;
        }
        if (it != tables.end()) {
            it->second.reset();
            tables.erase(it);
        }
        lua_createtable(L, 0, 4);
        LuaRef ref;
        ref.reset(L, -1);
        tables.emplace(node, std::move(ref));
    }

    void Fields::evict(cocos2d::CCNode* node) {
        if (!node) return;
        auto& tables = fieldTables();
        auto it = tables.find(node);
        if (it == tables.end()) return;
        it->second.reset();
        tables.erase(it);
    }

    void Fields::evict(cocos2d::CCObject* object) {
        if (!object) return;
        if (auto* node = geode::cast::typeinfo_cast<cocos2d::CCNode*>(object)) {
            evict(node);
        }
    }

    void Fields::evictIfFinalRelease(cocos2d::CCObject* object) {
        if (!object || object->retainCount() > 1) return;
        evict(object);
    }

    void Fields::clear() {
        auto& tables = fieldTables();
        for (auto& [_, ref] : tables) {
            ref.reset();
        }
        tables.clear();
    }
}
