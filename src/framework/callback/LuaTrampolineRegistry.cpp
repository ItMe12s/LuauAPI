#include "framework/callback/LuaTrampolineRegistry.hpp"

#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/lifecycle/Lifecycle.hpp"

#include <Geode/Geode.hpp>
#include <unordered_map>
#include <vector>

namespace luax {
    namespace {
        std::vector<cocos2d::CCObject*>& orphanTrampolines() {
            static std::vector<cocos2d::CCObject*> orphans;
            return orphans;
        }

        std::unordered_map<cocos2d::CCObject*, std::vector<cocos2d::CCObject*>>& anchorMap() {
            static std::unordered_map<cocos2d::CCObject*, std::vector<cocos2d::CCObject*>> anchors;
            return anchors;
        }

        bool s_shutdownHookRegistered = false;
        bool s_orphanCapWarned = false;

        void clearOrphanTrampolinesImpl() {
            if (Runtime::isShuttingDown()) {
                orphanTrampolines().clear();
                anchorMap().clear();
                return;
            }
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

        void clearOrphanTrampolinesOnShutdown() {
            clearOrphanTrampolinesImpl();
            s_shutdownHookRegistered = false;
        }
    } // namespace

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
                    "orphan trampoline registry exceeded cap ({}), dropping "
                    "new trampolines",
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
        clearOrphanTrampolinesImpl();
    }

    void ensureTrampolineShutdownHook() {
        ensureShutdownHook(s_shutdownHookRegistered, &clearOrphanTrampolinesOnShutdown);
    }
} // namespace luax
