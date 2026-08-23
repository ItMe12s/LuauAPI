#pragma once

#include "bindings/lunar/LunarCCAction.hpp"
#include "bindings/lunar/LunarModel.hpp"
#include "bindings/lunar/LunarRig.hpp"
#include "framework/callback/LuaCallback.hpp"

#include <Geode/Result.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

struct lua_State;

namespace luax::lunar {

    class LunarAnimationDef final : public cocos2d::CCObject {
    public:
        static LunarAnimationDef* create();

        std::vector<Keyframe> const& keyframes() const noexcept {
            return m_keyframes;
        }

        void setKeyframes(std::vector<Keyframe> keyframes) noexcept {
            m_keyframes = std::move(keyframes);
        }

        double fps() const noexcept {
            return m_fps;
        }

        void setFps(double fps) noexcept {
            m_fps = fps;
        }

        bool looped() const noexcept {
            return m_looped;
        }

        void setLooped(bool looped) noexcept {
            m_looped = looped;
        }

        void addKeyframe(double frame, std::string_view nodeId, NodePose pose);

        void addEvent(double frame, std::string name);

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
        void bindEvent(std::string name, LuaCallback callback);

        bool isPlaying() const noexcept {
            return m_playing;
        }

        bool isPaused() const noexcept {
            return m_paused;
        }

        float speed() const noexcept {
            return m_speed;
        }

        double duration() const;

        std::unordered_map<std::string, NodePose> sample(double time);

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

        struct EventBind {
            std::string name;
            LuaCallback callback;
        };

        LunarTrack() = default;

        void launch(double fromTime);
        void stopActions();
        void applyDueInstants();
        void applyDueEvents();
        void finish();

        geode::Ref<LunarRig> m_rig;
        CompiledAnimation m_anim;
        CompiledAnimation const* m_active = nullptr;
        CompiledAnimation m_sliced;
        std::vector<geode::Ref<cocos2d::CCNode>> m_launched;
        std::vector<geode::Ref<LunarCCSpeed>> m_tweens;
        std::vector<TargetInstants> m_instants;
        std::vector<EventBind> m_eventBinds;
        double m_launchBase = 0.0;
        double m_elapsed = 0.0;
        float m_speed = 1.F;
        bool m_playing = false;
        bool m_paused = false;
        int m_tag = 0;
        std::size_t m_eventCursor = 0;
    };

    geode::Result<void> registerLunarAnimation(lua_State* L);

    void shutdownLunarTracks();

    geode::Result<LunarAnimationDef*> parseAnimTable(lua_State* L, int idx, char const* method);

    bool setNodeOpacity(cocos2d::CCNode* node, float value);

    void applyPose(LunarRig* rig, std::unordered_map<std::string, NodePose> const& poses);

} // namespace luax::lunar
