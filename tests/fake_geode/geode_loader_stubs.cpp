#include <Geode/loader/Tulip.hpp>
#include <Geode/utils/cocos.hpp>

#include <cocos2d.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace {
    thread_local std::vector<void*> s_destructorLockStack;
}

namespace geode {
    bool WeakRefController::isManaged() {
        WeakRefPool::get()->check(m_obj);
        return m_obj != nullptr;
    }

    cocos2d::CCObject* WeakRefController::get() const {
        return m_obj;
    }

    WeakRefPool* WeakRefPool::get() {
        static auto* inst = new WeakRefPool();
        return inst;
    }

    void WeakRefPool::check(cocos2d::CCObject* obj) {
        if (obj && obj->retainCount() == 1) {
            this->forget(obj);
        }
    }

    void WeakRefPool::forget(cocos2d::CCObject* obj) {
        if (!obj || !m_pool.contains(obj)) {
            return;
        }

        obj->release();
        m_pool.at(obj)->m_obj = nullptr;
        m_pool.erase(obj);
    }

    std::shared_ptr<WeakRefController> WeakRefPool::manage(cocos2d::CCObject* obj) {
        if (!obj) {
            return {};
        }

        if (!m_pool.contains(obj)) {
            obj->retain();
            auto controller = std::make_shared<WeakRefController>();
            controller->m_obj = obj;
            m_pool.insert({ obj, controller });
        }

        return m_pool.at(obj);
    }

    bool DestructorLock::isLocked(cocos2d::CCNode* self) {
        return isLocked(static_cast<void*>(self));
    }

    bool DestructorLock::isLocked(void* self) {
        if (s_destructorLockStack.empty()) {
            return false;
        }
        return s_destructorLockStack.back() == self;
    }

    void DestructorLock::addLock(cocos2d::CCNode* self) {
        addLock(static_cast<void*>(self));
    }

    void DestructorLock::addLock(void* self) {
        s_destructorLockStack.push_back(self);
    }

    void DestructorLock::removeLock(cocos2d::CCNode* self) {
        removeLock(static_cast<void*>(self));
    }

    void DestructorLock::removeLock(void* self) {
        if (s_destructorLockStack.empty() || s_destructorLockStack.back() != self) {
            return;
        }
        s_destructorLockStack.pop_back();
    }

    Result<void*> hook::createWrapper(
        void*,
        tulip::hook::WrapperMetadata const&
    ) noexcept {
        return Err("fake_geode: createWrapper unavailable in host tests");
    }

    std::shared_ptr<tulip::hook::CallingConvention> hook::createConvention(
        tulip::hook::TulipConvention
    ) noexcept {
        return nullptr;
    }
}
