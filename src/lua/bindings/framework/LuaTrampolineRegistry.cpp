#include "LuaTrampolineRegistry.hpp"

#include "lua/Config.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/Geode.hpp>

#include <unordered_map>
#include <vector>

namespace luax {
    namespace {
        std::vector<cocos2d::CCObject*>& orphanTrampolines() {
            // Cleared via shutdown hook.
            static auto* s_orphans = new std::vector<cocos2d::CCObject*>();
            return *s_orphans;
        }

        std::unordered_map<cocos2d::CCObject*, std::vector<cocos2d::CCObject*>>& anchorMap() {
            // Cleared by shutdown hook.
            static auto* s_anchors =
                new std::unordered_map<cocos2d::CCObject*, std::vector<cocos2d::CCObject*>>();
            return *s_anchors;
        }

        bool s_shutdownHookRegistered = false;
        bool s_orphanCapWarned = false;
    }

    void anchorTrampoline(cocos2d::CCObject* anchor, cocos2d::CCObject* trampoline) {
        if (!trampoline) return;
        if (!anchor) {
            registerOrphanTrampoline(trampoline);
            return;
        }
        trampoline->retain();
        anchorMap()[anchor].push_back(trampoline);
        ensureTrampolineShutdownHook();
    }

    void evictTrampolinesIfFinalRelease(cocos2d::CCObject* anchor) {
        if (!anchor || anchor->retainCount() > 1) return;
        auto& map = anchorMap();
        auto it = map.find(anchor);
        if (it == map.end()) return;
        for (auto* trampoline : it->second) {
            trampoline->release();
        }
        map.erase(it);
    }

    void registerOrphanTrampoline(cocos2d::CCObject* trampoline) {
        if (!trampoline) return;
        if (orphanTrampolines().size() >= kMaxCallbackTrampolines) {
            if (!s_orphanCapWarned) {
                s_orphanCapWarned = true;
                geode::log::warn(
                    "orphan trampoline registry exceeded cap ({}), dropping new trampolines",
                    kMaxCallbackTrampolines
                );
            }
            return;
        }
        trampoline->retain();
        orphanTrampolines().push_back(trampoline);
        ensureTrampolineShutdownHook();
    }

    void clearOrphanTrampolines() {
        for (auto* trampoline : orphanTrampolines()) {
            trampoline->release();
        }
        orphanTrampolines().clear();
        for (auto& [_, trampolines] : anchorMap()) {
            for (auto* trampoline : trampolines) {
                trampoline->release();
            }
        }
        anchorMap().clear();
    }

    void ensureTrampolineShutdownHook() {
        if (s_shutdownHookRegistered) return;
        auto* runtime = Runtime::getIfInitialized();
        if (!runtime) return;
        runtime->registerShutdownHook(&clearOrphanTrampolines);
        s_shutdownHookRegistered = true;
    }
}
