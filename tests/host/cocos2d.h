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
}

namespace cocos2d {
    class CCObject {
    public:
        CCObject() {
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

    private:
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
        explicit WeakRef(T* ptr) : m_ptr(ptr) {}

        Lock lock() const {
            if (!m_ptr || !detail::isLiveCocosObject(m_ptr)) {
                return Lock{};
            }
            return Lock{m_ptr};
        }

    private:
        T* m_ptr = nullptr;
    };

    namespace cast {
        template <class T, class U>
        T typeinfo_cast(U* value) {
            return dynamic_cast<T>(value);
        }
    }
}
