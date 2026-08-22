#pragma once

#include <cmath>
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

    protected:
        ~LunarCCSpeed() override {
            CC_SAFE_RELEASE(m_inner);
        }

    private:
        cocos2d::CCActionInterval* m_inner = nullptr;
        float m_speed = 1.F;
    };

    // Cocos rewrite both axes of their point every frame (CCMoveTo, CCScaleTo),
    // which would let concurrent per-axis chains override each other.
    // This tweens exactly one axis.
    class LunarCCAxisTo final : public cocos2d::CCActionInterval {
    public:
        enum class Axis : std::uint8_t {
            PosX,
            PosY,
            ScaleX,
            ScaleY,
            AnchorX,
            AnchorY,
        };

        static LunarCCAxisTo* create(float duration, Axis axis, float value) {
            auto* ret = new LunarCCAxisTo();
            if (ret->initWithDuration(duration)) {
                ret->m_axis = axis;
                ret->m_value = value;
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }

        void startWithTarget(cocos2d::CCNode* target) override {
            CCActionInterval::startWithTarget(target);
            m_start = target ? read(target) : 0.F;
        }

        void update(float t) override {
            if (!m_pTarget) return;
            write(m_pTarget, std::lerp(m_start, m_value, t));
        }

    protected:
        ~LunarCCAxisTo() override = default;

    private:
        float read(cocos2d::CCNode* target) const {
            switch (m_axis) {
                case Axis::PosX: return target->getPositionX();
                case Axis::PosY: return target->getPositionY();
                case Axis::ScaleX: return target->getScaleX();
                case Axis::ScaleY: return target->getScaleY();
                case Axis::AnchorX: return target->getAnchorPoint().x;
                case Axis::AnchorY: return target->getAnchorPoint().y;
            }
            return 0.F;
        }

        void write(cocos2d::CCNode* target, float value) const {
            switch (m_axis) {
                case Axis::PosX: target->setPositionX(value); break;
                case Axis::PosY: target->setPositionY(value); break;
                case Axis::ScaleX: target->setScaleX(value); break;
                case Axis::ScaleY: target->setScaleY(value); break;
                case Axis::AnchorX:
                    target->setAnchorPoint({value, target->getAnchorPoint().y});
                    break;
                case Axis::AnchorY:
                    target->setAnchorPoint({target->getAnchorPoint().x, value});
                    break;
            }
        }

        Axis m_axis = Axis::PosX;
        float m_value = 0.F;
        float m_start = 0.F;
    };
} // namespace luax::lunar
