#pragma once

#include <cstdint>
#include <unordered_set>

namespace cocos2d {
    class CCObject;
}

namespace geode::detail {
    inline std::unordered_set<cocos2d::CCObject const*>& liveCocosObjects() {
        static std::unordered_set<cocos2d::CCObject const*> objects;
        return objects;
    }

    inline void trackCocosObject(cocos2d::CCObject const* object) {
        if (object) {
            liveCocosObjects().insert(object);
        }
    }

    inline void untrackCocosObject(cocos2d::CCObject const* object) {
        if (object) {
            liveCocosObjects().erase(object);
        }
    }

    inline bool isLiveCocosObject(cocos2d::CCObject const* object) {
        return object && liveCocosObjects().contains(object);
    }

    inline bool& weakRefSimulateForgetForTests() {
        static bool enabled = false;
        return enabled;
    }
}

namespace cocos2d {
    class CCObject {
    public:
        CCObject() : m_objectId(++s_nextObjectId) {
            geode::detail::trackCocosObject(this);
        }

        virtual ~CCObject() {
            geode::detail::untrackCocosObject(this);
        }

        void retain() { ++m_retainCount; }
        void release() {
            if (m_retainCount > 0) {
                --m_retainCount;
            }
            if (m_retainCount == 0) {
                delete this;
            }
        }

        unsigned int retainCount() const { return m_retainCount; }
        std::uint64_t objectId() const { return m_objectId; }

    private:
        static inline std::uint64_t s_nextObjectId = 0;
        std::uint64_t m_objectId = 0;
        unsigned int m_retainCount = 1;
    };

    class CCNode : public CCObject {};

    class CCScheduler {
    public:
        void scheduleUpdateForTarget(CCNode*, int, bool) {}
        void unscheduleUpdateForTarget(CCNode*) {}
    };

    class CCDirector {
    public:
        static CCDirector* sharedDirector() {
            static CCDirector director;
            return &director;
        }

        CCScheduler* getScheduler() { return &m_scheduler; }

    private:
        CCScheduler m_scheduler;
    };
}

namespace geode {
    template <class T>
    class WeakRef {
    public:
        struct Lock {
            explicit operator bool() const { return ptr != nullptr; }

            T* data() const { return ptr; }
            T* ptr = nullptr;
        };

        WeakRef() = default;
        explicit WeakRef(T* ptr) : m_ptr(ptr), m_objectId(ptr ? ptr->objectId() : 0) {}

        WeakRef(WeakRef&& other) noexcept
            : m_ptr(other.m_ptr), m_objectId(other.m_objectId) {
            other.m_ptr = nullptr;
            other.m_objectId = 0;
        }

        WeakRef& operator=(WeakRef&& other) noexcept {
            if (this != &other) {
                forget();
                m_ptr = other.m_ptr;
                m_objectId = other.m_objectId;
                other.m_ptr = nullptr;
                other.m_objectId = 0;
            }
            return *this;
        }

        WeakRef(WeakRef const&) = delete;
        WeakRef& operator=(WeakRef const&) = delete;

        ~WeakRef() { forget(); }

        Lock lock() const {
            if (!m_ptr || !detail::isLiveCocosObject(m_ptr)) {
                return Lock{};
            }
            if (m_ptr->objectId() != m_objectId) {
                return Lock{};
            }
            return Lock{m_ptr};
        }

        bool valid() const { return static_cast<bool>(lock()); }

    private:
        void forget() {
            if (!m_ptr || !detail::isLiveCocosObject(m_ptr)) {
                m_ptr = nullptr;
                m_objectId = 0;
                return;
            }
            if (m_ptr->objectId() != m_objectId) {
                m_ptr = nullptr;
                m_objectId = 0;
                return;
            }
            if (detail::weakRefSimulateForgetForTests()) {
                m_ptr->release();
            }
            m_ptr = nullptr;
            m_objectId = 0;
        }

        T* m_ptr = nullptr;
        std::uint64_t m_objectId = 0;
    };

    namespace cast {
        template <class T, class U>
        T typeinfo_cast(U* value) {
            return dynamic_cast<T>(value);
        }
    }
}
