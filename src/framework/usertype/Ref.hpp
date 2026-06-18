#pragma once

#include "core/Runtime.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <unordered_map>

namespace luax {
    inline std::unordered_map<cocos2d::CCObject*, unsigned int>& luaRetains() {
        static std::unordered_map<cocos2d::CCObject*, unsigned int> value;
        return value;
    }

    inline void clearLuaRetains() {
        auto& retains = luaRetains();
        if (Runtime::isShuttingDown()) {
            retains.clear();
            return;
        }
        for (auto const& entry : retains) {
            for (unsigned int i = 0; i < entry.second; ++i) {
                entry.first->release();
            }
        }
        retains.clear();
    }

    inline bool assertMainThread() {
        if (luax::Runtime::isShuttingDown()) return false;
        auto* runtime = luax::Runtime::getOrCreate();
        return runtime && runtime->assertMainThread();
    }

    inline void untrackLuaRetain(cocos2d::CCObject* object) {
        if (!object) return;
        auto& retains = luaRetains();
        auto found = retains.find(object);
        if (found == retains.end() || found->second == 0) return;
        found->second -= 1;
        if (found->second == 0) {
            retains.erase(found);
        }
    }

    inline bool retainLuaRef(cocos2d::CCObject* object, char const* method) {
        if (!object) {
            geode::log::error("[lua:{}] expected CCObject", method);
            return false;
        }
        if (!assertMainThread()) return false;
        object->retain();
        luaRetains()[object] += 1;
        return true;
    }

} // namespace luax
