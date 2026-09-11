#pragma once

#include "bindings/lunar/LunarModel.hpp"

#include <Geode/utils/casts.hpp>
#include <array>
#include <cmath>
#include <cocos2d.h>
#include <optional>

// CCSpeed is not on iOS because of tree shaking so here's one for lunar.
// https://github.com/cocos2d/cocos2d-x/blob/cocos2d-x-2.2.3/cocos2dx/actions/CCAction.cpp

namespace luax::lunar {

    struct PropNodeAccess {
        std::optional<float> (*getter)(cocos2d::CCNode*);
        bool (*setter)(cocos2d::CCNode*, float);
    };

    // Aligned with kPropFields (LunarModel.hpp) in exact Prop order.
    inline constexpr std::array<PropNodeAccess, 11> kPropNodeAccess{{
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getPositionX();
         },
         [](cocos2d::CCNode* n, float v) {
             n->setPositionX(v);
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getPositionY();
         },
         [](cocos2d::CCNode* n, float v) {
             n->setPositionY(v);
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getRotation();
         },
         [](cocos2d::CCNode* n, float v) {
             n->setRotation(v);
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getScaleX();
         },
         [](cocos2d::CCNode* n, float v) {
             n->setScaleX(v);
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getScaleY();
         },
         [](cocos2d::CCNode* n, float v) {
             n->setScaleY(v);
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             if (auto* rgba = geode::cast::typeinfo_cast<cocos2d::CCRGBAProtocol*>(n)) {
                 return static_cast<float>(rgba->getOpacity());
             }
             return std::nullopt;
         },
         [](cocos2d::CCNode* n, float v) -> bool {
             if (auto* rgba = geode::cast::typeinfo_cast<cocos2d::CCRGBAProtocol*>(n)) {
                 rgba->setOpacity(opacityByte(v));
                 return true;
             }
             return false;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return static_cast<float>(n->getZOrder());
         },
         [](cocos2d::CCNode* n, float v) {
             n->setZOrder(static_cast<int>(v));
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getAnchorPoint().x;
         },
         [](cocos2d::CCNode* n, float v) {
             n->setAnchorPoint({v, n->getAnchorPoint().y});
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getAnchorPoint().y;
         },
         [](cocos2d::CCNode* n, float v) {
             n->setAnchorPoint({n->getAnchorPoint().x, v});
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getSkewX();
         },
         [](cocos2d::CCNode* n, float v) {
             n->setSkewX(v);
             return true;
         }},
        {[](cocos2d::CCNode* n) -> std::optional<float> {
             return n->getSkewY();
         },
         [](cocos2d::CCNode* n, float v) {
             n->setSkewY(v);
             return true;
         }},
    }};

    constexpr auto kPropNodeAccessAligned = [] {
        for (std::size_t i = 0; i < kPropFields.size(); ++i)
            if (kPropFields[i].prop != static_cast<Prop>(i)) return false;
        return true;
    }();
    static_assert(
        kPropNodeAccess.size() == kPropFields.size() && kPropNodeAccessAligned,
        "kPropNodeAccess must stay aligned with kPropFields"
    );

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
        static LunarCCAxisTo* create(float duration, Prop prop, float value) {
            auto* ret = new LunarCCAxisTo();
            if (ret->initWithDuration(duration)) {
                ret->m_prop = prop;
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
            return kPropNodeAccess[static_cast<std::size_t>(m_prop)].getter(target).value_or(0.F);
        }

        void write(cocos2d::CCNode* target, float value) const {
            kPropNodeAccess[static_cast<std::size_t>(m_prop)].setter(target, value);
        }

        Prop m_prop = Prop::PosX;
        float m_value = 0.F;
        float m_start = 0.F;
    };
} // namespace luax::lunar
