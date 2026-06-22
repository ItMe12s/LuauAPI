#include "framework/usertype/Fields.hpp"

#include "core/Runtime.hpp"
#include "framework/usertype/WeakRefShutdown.hpp"

#include <Geode/Geode.hpp>
#include <unordered_map>

namespace luax {
    namespace {
        struct FieldEntry {
            LuaRef table;
            geode::WeakRef<cocos2d::CCNode> owner;
        };

        std::unordered_map<cocos2d::CCNode*, FieldEntry>& fieldTables() {
            static std::unordered_map<cocos2d::CCNode*, FieldEntry> tables;
            return tables;
        }

        bool entryStillOwnsNode(FieldEntry const& entry, cocos2d::CCNode* node) {
            return node && entry.owner.valid();
        }
    } // namespace

    bool Fields::tryPush(lua_State* L, cocos2d::CCNode* node) {
        if (!L || !node) {
            return false;
        }

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
        if (Runtime::isShuttingDown()) {
            detail::parkWeakRefForPoolSafety(std::move(it->second.owner));
        }
        tables.erase(it);
    }

    void Fields::evict(cocos2d::CCObject* object) {
        if (!object) return;
        auto* node = geode::cast::typeinfo_cast<cocos2d::CCNode*>(object);
        if (!node) return;
        evict(node);
    }

    void Fields::evictIfFinalRelease(cocos2d::CCObject* object) {
        if (!object) return;
        auto* node = geode::cast::typeinfo_cast<cocos2d::CCNode*>(object);
        if (!node) return;
        auto& tables = fieldTables();
        auto it = tables.find(node);
        if (it == tables.end() || !it->second.owner.valid()) return;
        if (object->retainCount() > 1) return;
        evict(it->first);
    }

    void Fields::clear() {
        auto& tables = fieldTables();
        if (Runtime::isShuttingDown()) {
            for (auto& [_, entry] : tables) {
                entry.table.reset();
                detail::parkWeakRefForPoolSafety(std::move(entry.owner));
            }
            tables.clear();
            return;
        }
        for (auto& [_, entry] : tables) {
            entry.table.reset();
        }
        tables.clear();
    }
} // namespace luax
