#pragma once

#include <cocos2d.h>

// CCSpeed is not on iOS because of tree shaking so here's one for lunar.
// https://github.com/cocos2d/cocos2d-x/blob/cocos2d-x-2.2.3/cocos2dx/actions/CCAction.cpp

namespace luax::lunar {

    class LunarCCSpeed final : public cocos2d::CCActionInterval {
    public:
        static LunarCCSpeed* create(cocos2d::CCActionInterval* action, float speed) {
            auto* ret = new LunarCCSpeed();
            if (ret->initWithAction(action, speed)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }

        bool initWithAction(cocos2d::CCActionInterval* action, float speed) {
            if (!action) return false;
            action->retain();
            m_inner = action;
            m_speed = speed;
            return true;
        }

        void setSpeed(float speed) noexcept {
            m_speed = speed;
        }

        float getSpeed() const noexcept {
            return m_speed;
        }

        cocos2d::CCActionInterval* getInnerAction() const noexcept {
            return m_inner;
        }

        void startWithTarget(cocos2d::CCNode* target) override {
            m_pOriginalTarget = m_pTarget = target;
            m_inner->startWithTarget(target);
        }

        void stop() override {
            m_inner->stop();
            m_pTarget = nullptr;
        }

        void step(float dt) override {
            m_inner->step(dt * m_speed);
        }

        bool isDone() override {
            return m_inner->isDone();
        }

        cocos2d::CCActionInterval* reverse() override {
            return create(m_inner->reverse(), m_speed);
        }

    protected:
        ~LunarCCSpeed() override {
            CC_SAFE_RELEASE(m_inner);
        }

    private:
        cocos2d::CCActionInterval* m_inner = nullptr;
        float m_speed = 1.F;
    };
} // namespace luax::lunar
