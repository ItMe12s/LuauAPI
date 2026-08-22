#pragma once

#include "bindings/lunar/LunarModel.hpp"
#include "bindings/lunar/LunarRig.hpp"

#include <Geode/Result.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <cstddef>
#include <vector>

struct lua_State;

namespace luax::lunar {

    class LunarAnimationDef final : public cocos2d::CCObject {
    public:
        static LunarAnimationDef* create();

        std::vector<Keyframe> const& keyframes() const {
            return m_keyframes;
        }

        void setKeyframes(std::vector<Keyframe> keyframes) {
            m_keyframes = std::move(keyframes);
        }

        double fps() const {
            return m_fps;
        }

        void setFps(double fps) {
            m_fps = fps;
        }

        bool looped() const {
            return m_looped;
        }

        void setLooped(bool looped) {
            m_looped = looped;
        }

        void addKeyframe(double frame, std::string_view nodeId, NodePose pose);

    private:
        ~LunarAnimationDef() override = default;

        std::vector<Keyframe> m_keyframes;
        double m_fps = 30.0;
        bool m_looped = false;
    };

    class LunarTrack final : public cocos2d::CCObject {
    public:
        static LunarTrack* create(LunarRig* rig, CompiledAnimation anim);

        void play();
        void pause();
        void unpause();
        void stop();
        void setSpeed(float speed);

        bool isPlaying() const {
            return m_playing;
        }

        bool isPaused() const {
            return m_paused;
        }

        float speed() const {
            return m_speed;
        }

        double duration() const;

        void tick(float dt);

        void detachForShutdown();

    protected:
        ~LunarTrack() override;

    private:
        struct TimedSet {
            double time;
            Prop prop;
            float value;
        };

        struct TargetInstants {
            geode::Ref<cocos2d::CCNode> node;
            std::vector<TimedSet> sets;
            std::size_t cursor = 0;
        };

        LunarTrack() = default;

        void launch(double fromTime);
        void stopActions();
        void applyDueInstants();
        void finish();

        geode::Ref<LunarRig> m_rig;
        CompiledAnimation m_anim;
        CompiledAnimation const* m_active = nullptr;
        CompiledAnimation m_sliced;
        std::vector<geode::Ref<cocos2d::CCNode>> m_launched;
        std::vector<geode::Ref<cocos2d::CCSpeed>> m_tweens;
        std::vector<TargetInstants> m_instants;
        double m_launchBase = 0.0;
        double m_elapsed = 0.0;
        float m_speed = 1.F;
        bool m_playing = false;
        bool m_paused = false;
        int m_tag = 0;
    };

    geode::Result<void> registerLunarAnimation(lua_State* L);

    void shutdownLunarTracks();

    geode::Result<LunarAnimationDef*> parseAnimTable(lua_State* L, int idx, char const* method);

} // namespace luax::lunar
