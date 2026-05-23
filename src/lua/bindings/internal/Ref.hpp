#pragma once

#include "../../Runtime.hpp"

#include <cocos2d.h>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace luax {
    inline std::unordered_map<cocos2d::CCObject*, unsigned int>& luaRetains() {
        static auto* value = new std::unordered_map<cocos2d::CCObject*, unsigned int>();
        return *value;
    }

    inline void assertMainThread() {
        luax::Runtime::instance().assertMainThread();
    }

    inline void releaseLuaRetain(cocos2d::CCObject* object, char const* method, bool explicitRelease) {
        if (!object) return;
        auto& retains = luaRetains();
        auto found = retains.find(object);
        if (found == retains.end() || found->second == 0) {
            if (explicitRelease) {
                throw std::runtime_error(std::string(method) + " has no Lua-owned retain");
            }
            return;
        }
        found->second -= 1;
        if (found->second == 0) {
            retains.erase(found);
        }
        object->release();
    }

    inline void retainLuaRef(cocos2d::CCObject* object, char const* method) {
        if (!object) throw std::runtime_error(std::string(method) + " expected CCObject");
        assertMainThread();
        object->retain();
        luaRetains()[object] += 1;
    }

    inline void releaseLuaRef(cocos2d::CCObject* object, char const* method) {
        assertMainThread();
        releaseLuaRetain(object, method, true);
    }
}
