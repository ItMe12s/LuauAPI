#include "Fields.hpp"

#include <Geode/Geode.hpp>
#include <unordered_map>

namespace luax {
    namespace {
        struct FieldEntry {
            LuaRef table;
            geode::WeakRef<cocos2d::CCNode> owner;
        };

        std::unordered_map<cocos2d::CCNode*, FieldEntry>& fieldTables() {
            static auto* value = new std::unordered_map<cocos2d::CCNode*, FieldEntry>();
            return *value;
        }

        bool entryStillOwnsNode(FieldEntry const& entry, cocos2d::CCNode* node) {
            auto lock = entry.owner.lock();
            return lock && lock.data() == node;
        }

        void purgeStaleFieldEntries() {
            auto& tables = fieldTables();
            for (auto it = tables.begin(); it != tables.end();) {
                auto lock = it->second.owner.lock();
                if (!lock || lock.data() != it->first) {
                    it->second.table.reset();
                    it = tables.erase(it);
                    continue;
                }
                ++it;
            }
        }
    } // namespace

    bool Fields::tryPush(lua_State* L, cocos2d::CCNode* node) {
        if (!L || !node) {
            return false;
        }

        purgeStaleFieldEntries();

        auto& tables = fieldTables();
        auto it = tables.find(node);
        if (it == tables.end()) {
            return false;
        }
        if (!entryStillOwnsNode(it->second, node)) {
            it->second.table.reset();
            tables.erase(it);
            return false;
        }
        return it->second.table.push();
    }

    void Fields::push(lua_State* L, cocos2d::CCNode* node) {
        if (!L) return;
        if (!node) {
            lua_pushnil(L);
            return;
        }

        purgeStaleFieldEntries();

        auto& tables = fieldTables();
        auto it = tables.find(node);
        if (it != tables.end()) {
            if (entryStillOwnsNode(it->second, node) && it->second.table.push()) {
                return;
            }
            it->second.table.reset();
            tables.erase(it);
        }

        lua_createtable(L, 0, 4);
        FieldEntry entry;
        entry.table.reset(L, -1);
        entry.owner = geode::WeakRef<cocos2d::CCNode>(node);
        tables.emplace(node, std::move(entry));
    }

    void Fields::evict(cocos2d::CCNode* node) {
        if (!node) return;
        auto& tables = fieldTables();
        auto it = tables.find(node);
        if (it == tables.end()) return;
        it->second.table.reset();
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
        for (auto& [_, entry] : tables) {
            entry.table.reset();
        }
        tables.clear();
    }
} // namespace luax
