#pragma once

#include "../../Runtime.hpp"

#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <string>
#include <unordered_map>

namespace luax {
    inline std::unordered_map<cocos2d::CCObject*, unsigned int>& luaRetains() {
        static auto* value = new std::unordered_map<cocos2d::CCObject*, unsigned int>();
        return *value;
    }

    inline bool assertMainThread() {
        return luax::Runtime::instance().assertMainThread();
    }

    inline void releaseLuaRetain(cocos2d::CCObject* object, char const* method, bool explicitRelease) {
        if (!object) return;
        auto& retains = luaRetains();
        auto found = retains.find(object);
        if (found == retains.end() || found->second == 0) {
            if (explicitRelease) {
                geode::log::error("[lua:{}] has no Lua-owned retain", method);
            }
            return;
        }
        found->second -= 1;
        if (found->second == 0) {
            retains.erase(found);
        }
        object->release();
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

    inline void releaseLuaRef(cocos2d::CCObject* object, char const* method) {
        if (!assertMainThread()) return;
        releaseLuaRetain(object, method, true);
    }
}
