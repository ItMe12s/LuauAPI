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

        constexpr std::size_t kLazyPurgeInterval = 64;

        std::unordered_map<cocos2d::CCNode*, FieldEntry>& fieldTables() {
            static std::unordered_map<cocos2d::CCNode*, FieldEntry> tables;
            return tables;
        }

        std::size_t& fieldAccessCounter() {
            static std::size_t counter = 0;
            return counter;
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

        void maybePurgeStaleFieldEntries() {
            auto& counter = fieldAccessCounter();
            if (++counter % kLazyPurgeInterval != 0) {
                return;
            }
            purgeStaleFieldEntries();
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

        maybePurgeStaleFieldEntries();

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
        if (auto* node = geode::cast::typeinfo_cast<cocos2d::CCNode*>(object)) {
            evict(node);
        }
    }

    void Fields::evictIfFinalRelease(cocos2d::CCObject* object) {
        if (!object) return;
        auto& tables = fieldTables();
        auto it = tables.find(reinterpret_cast<cocos2d::CCNode*>(object));
        if (it == tables.end() || !entryStillOwnsNode(it->second, it->first)) return;
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
