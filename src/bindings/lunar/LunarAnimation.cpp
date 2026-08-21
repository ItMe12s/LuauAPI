#include "bindings/lunar/LunarAnimation.hpp"

#include "bindings/lunar/LunarRig.hpp"
#include "core/Runtime.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/usertype/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <atomic>
#include <fmt/format.h>
#include <lua.h>
#include <lualib.h>

namespace luax::lunar {
    namespace {

        constexpr double kTimeEps = 1e-9;

        GLubyte opacityByte(float v) {
            return static_cast<GLubyte>(std::clamp(v, 0.F, 255.F));
        }

        Keyframe& keyframeFor(std::vector<Keyframe>& keyframes, double frame) {
            auto it = std::lower_bound(
                keyframes.begin(), keyframes.end(), frame, [](Keyframe const& kf, double f) {
                    return kf.frame < f;
                }
            );
            if (it != keyframes.end() && std::fabs(it->frame - frame) < kTimeEps) return *it;
            Keyframe kf;
            kf.frame = frame;
            return *keyframes.insert(it, std::move(kf));
        }

        void setPoseTarget(Keyframe& kf, std::string_view nodeId, NodePose pose) {
            for (auto& [id, existing] : kf.targets) {
                if (id == nodeId) {
                    existing = std::move(pose);
                    return;
                }
            }
            kf.targets.emplace_back(std::string(nodeId), std::move(pose));
        }

        geode::Result<NodePose> parseNodePose(lua_State* L, int idx, std::string_view nodeId) {
            luaL_checktype(L, idx, LUA_TTABLE);
            NodePose pose;
            lua_pushnil(L);
            while (lua_next(L, idx) != 0) {
                if (lua_type(L, -2) != LUA_TSTRING) {
                    lua_pop(L, 1);
                    return geode::Err(
                        fmt::format("keyframe property keys must be strings (node '{}')", nodeId)
                    );
                }
                size_t len = 0;
                char const* key = lua_tolstring(L, -2, &len);
                std::string_view const name(key, len);
                bool const isNumber = lua_isnumber(L, -1) != 0;

                if (isNumber) {
                    float const value = static_cast<float>(lua_tonumber(L, -1));
                    if (name == "x") pose.x = value;
                    else if (name == "y") pose.y = value;
                    else if (name == "rot") pose.rot = value;
                    else if (name == "sx") pose.sx = value;
                    else if (name == "sy") pose.sy = value;
                    else if (name == "opacity") pose.opacity = value;
                    else if (name == "z") pose.z = value;
                    else {
                        geode::log::warn("ignoring unknown animation property '{}'", name);
                    }
                }
                else if (name == "easing") {
                    if (lua_type(L, -1) != LUA_TSTRING) {
                        lua_pop(L, 1);
                        return geode::Err(fmt::format("'easing' must be a string (node '{}')", nodeId));
                    }
                    char const* easingName = lua_tostring(L, -1);
                    auto easing = easingFromString(easingName ? easingName : "");
                    if (!easing) {
                        lua_pop(L, 1);
                        return geode::Err(
                            fmt::format(
                                "unknown easing '{}' (node '{}')", easingName ? easingName : "?", nodeId
                            )
                        );
                    }
                    pose.easing = *easing;
                }
                else {
                    geode::log::warn("ignoring unknown animation property '{}'", name);
                }
                lua_pop(L, 1);
            }
            return geode::Ok(pose);
        }

        struct ParsedAnim {
            std::vector<Keyframe> keyframes;
            double fps = 30.0;
            bool looped = false;
        };

        geode::Result<ParsedAnim> parseAnimTable(lua_State* L, int idx, char const* method) {
            luaL_checktype(L, idx, LUA_TTABLE);
            ParsedAnim out;

            if (auto fps = optNumberField(L, idx, "fps", method)) {
                if (!(fps > 0.0)) return geode::Err(std::string("'fps' must be > 0"));
                out.fps = *fps;
            }
            if (auto looped = optBoolField(L, idx, "looped", method)) out.looped = *looped;

            lua_getfield(L, idx, "keyframes");
            if (!lua_istable(L, -1)) {
                lua_pop(L, 1);
                return geode::Err(std::string("'keyframes' table is required"));
            }
            int const kfsIdx = lua_gettop(L);
            lua_pushnil(L);
            while (lua_next(L, kfsIdx) != 0) {
                if (!lua_isnumber(L, -2)) {
                    lua_pop(L, 1);
                    return geode::Err(std::string("'keyframes' must be keyed by frame number"));
                }
                double const keyedFrame = lua_tonumber(L, -2);
                if (!lua_istable(L, -1)) {
                    lua_pop(L, 1);
                    return geode::Err(fmt::format("keyframes[{}] must be a table", keyedFrame));
                }
                int const entry = lua_gettop(L);
                double frame = keyedFrame;
                if (auto explicitFrame = optNumberField(L, entry, "frame", method)) {
                    frame = *explicitFrame;
                }

                lua_pushnil(L);
                while (lua_next(L, entry) != 0) {
                    if (lua_type(L, -2) == LUA_TSTRING &&
                        std::string_view(lua_tostring(L, -2)) == "frame") {
                        lua_pop(L, 1);
                        continue;
                    }
                    if (lua_type(L, -2) != LUA_TSTRING) {
                        lua_pop(L, 1);
                        return geode::Err(
                            fmt::format("keyframes[{}] entries must map node ids to tables", frame)
                        );
                    }
                    char const* nodeId = lua_tostring(L, -2);
                    auto poseResult = parseNodePose(L, -1, nodeId ? nodeId : "");
                    if (poseResult.isErr()) {
                        std::string err = poseResult.unwrapErr();
                        lua_pop(L, 1);
                        return geode::Err(std::move(err));
                    }
                    setPoseTarget(
                        keyframeFor(out.keyframes, frame),
                        nodeId ? nodeId : "",
                        std::move(poseResult).unwrap()
                    );
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
            return geode::Ok(std::move(out));
        }

        cocos2d::CCFiniteTimeAction* makeTween(TweenSeg const& seg, cocos2d::CCNode* node) {
            float const dur = static_cast<float>(std::max(0.0, seg.end - seg.start));
            using P = Prop;
            cocos2d::CCActionInterval* to = nullptr;
            switch (seg.prop) {
                case P::PosX:
                    to = cocos2d::CCMoveTo::create(dur, {seg.to, node->getPositionY()});
                    break;
                case P::PosY:
                    to = cocos2d::CCMoveTo::create(dur, {node->getPositionX(), seg.to});
                    break;
                case P::Rotation: to = cocos2d::CCRotateTo::create(dur, seg.to); break;
                case P::ScaleX:
                    to = cocos2d::CCScaleTo::create(dur, seg.to, node->getScaleY());
                    break;
                case P::ScaleY:
                    to = cocos2d::CCScaleTo::create(dur, node->getScaleX(), seg.to);
                    break;
                case P::Opacity: to = cocos2d::CCFadeTo::create(dur, opacityByte(seg.to)); break;
                case P::ZOrder: break; // Instant-only, never reaches here.
            }
            if (!to) return nullptr;

            using K = EasingKind;
            cocos2d::CCActionInterval* wrapped = nullptr;
            switch (seg.easing.kind) {
                case K::Linear: wrapped = to; break;
                case K::PowIn: wrapped = cocos2d::CCEaseIn::create(to, seg.easing.rate); break;
                case K::PowOut: wrapped = cocos2d::CCEaseOut::create(to, seg.easing.rate); break;
                case K::PowInOut:
                    wrapped = cocos2d::CCEaseInOut::create(to, seg.easing.rate);
                    break;
                case K::SineIn: wrapped = cocos2d::CCEaseSineIn::create(to); break;
                case K::SineOut: wrapped = cocos2d::CCEaseSineOut::create(to); break;
                case K::SineInOut: wrapped = cocos2d::CCEaseSineInOut::create(to); break;
                case K::ExpoIn: wrapped = cocos2d::CCEaseExponentialIn::create(to); break;
                case K::ExpoOut: wrapped = cocos2d::CCEaseExponentialOut::create(to); break;
                case K::ExpoInOut: wrapped = cocos2d::CCEaseExponentialInOut::create(to); break;
                case K::BackIn: wrapped = cocos2d::CCEaseBackIn::create(to); break;
                case K::BackOut: wrapped = cocos2d::CCEaseBackOut::create(to); break;
                case K::BackInOut: wrapped = cocos2d::CCEaseBackInOut::create(to); break;
                case K::ElasticIn: wrapped = cocos2d::CCEaseElasticIn::create(to); break;
                case K::ElasticOut: wrapped = cocos2d::CCEaseElasticOut::create(to); break;
                case K::ElasticInOut: wrapped = cocos2d::CCEaseElasticInOut::create(to); break;
                case K::BounceIn: wrapped = cocos2d::CCEaseBounceIn::create(to); break;
                case K::BounceOut: wrapped = cocos2d::CCEaseBounceOut::create(to); break;
                case K::BounceInOut: wrapped = cocos2d::CCEaseBounceInOut::create(to); break;
            }
            return wrapped;
        }

        void applyInstant(cocos2d::CCNode* node, Prop prop, float value) {
            using P = Prop;
            switch (prop) {
                case P::PosX: node->setPositionX(value); break;
                case P::PosY: node->setPositionY(value); break;
                case P::Rotation: node->setRotation(value); break;
                case P::ScaleX: node->setScaleX(value); break;
                case P::ScaleY: node->setScaleY(value); break;
                case P::Opacity: {
                    if (auto* rgba = dynamic_cast<cocos2d::CCRGBAProtocol*>(node)) {
                        rgba->setOpacity(opacityByte(value));
                    }
                    break;
                }
                case P::ZOrder: node->setZOrder(static_cast<int>(value)); break;
            }
        }

        std::vector<LunarTrack*>& liveTracks() {
            static std::vector<LunarTrack*> value;
            return value;
        }

        class LunarTickNode final : public cocos2d::CCNode {
        public:
            void update(float dt) override {
                for (auto* track : liveTracks()) {
                    if (track) track->tick(dt);
                }
            }
        };

        LunarTickNode*& tickNode() {
            static LunarTickNode* value = nullptr;
            return value;
        }

        void ensureTicker() {
            if (tickNode()) return;
            auto* node = new LunarTickNode();
            node->retain();
            tickNode() = node;
            cocos2d::CCDirector::sharedDirector()->getScheduler()->scheduleUpdateForTarget(
                node, 0, false
            );
        }

        int animNew(lua_State* L) {
            Usertype<LunarAnimationDef>::pushOwned(L, LunarAnimationDef::create());
            return 1;
        }

        int animLoad(lua_State* L) {
            auto parsed = parseAnimTable(L, 1, "animation.load");
            if (parsed.isErr()) {
                luaL_error(L, "animation.load: %s", parsed.unwrapErr().c_str());
            }
            auto value = std::move(parsed).unwrap();
            auto* def = LunarAnimationDef::create();
            def->setFps(value.fps);
            def->setLooped(value.looped);
            def->keyframes() = std::move(value.keyframes);
            Usertype<LunarAnimationDef>::pushOwned(L, def);
            return 1;
        }

        int animSetFps(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:setFps");
            double const fps = check<double>(L, 2, "LunarAnimationDef:setFps");
            if (!(fps > 0.0)) {
                luaL_error(L, "LunarAnimationDef:setFps expected fps > 0");
            }
            self->setFps(fps);
            return 0;
        }

        int animGetFps(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:getFps");
            push(L, self->fps());
            return 1;
        }

        int animSetLooped(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:setLooped");
            self->setLooped(check<bool>(L, 2, "LunarAnimationDef:setLooped"));
            return 0;
        }

        int animGetLooped(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:getLooped");
            push(L, self->looped());
            return 1;
        }

        int animAddKeyframe(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:addKeyframe");
            double const frame = check<double>(L, 2, "LunarAnimationDef:addKeyframe");
            if (!(frame >= 0.0)) {
                luaL_error(L, "LunarAnimationDef:addKeyframe expected frame >= 0");
            }
            auto const nodeId = check<std::string>(L, 3, "LunarAnimationDef:addKeyframe");
            auto pose = parseNodePose(L, 4, nodeId);
            if (pose.isErr()) {
                luaL_error(L, "LunarAnimationDef:addKeyframe: %s", pose.unwrapErr().c_str());
            }
            self->addKeyframe(frame, nodeId, std::move(pose).unwrap());
            return 0;
        }

        int trackPlay(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:play");
            self->play();
            return 0;
        }

        int trackPause(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:pause");
            self->pause();
            return 0;
        }

        int trackContinue(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:continue");
            self->continuePlayback();
            return 0;
        }

        int trackStop(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:stop");
            self->stop();
            return 0;
        }

        int trackSetSpeed(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:setSpeed");
            float const speed = check<float>(L, 2, "LunarAnimationTrack:setSpeed");
            if (!(speed > 0.F)) {
                luaL_error(L, "LunarAnimationTrack:setSpeed expected speed > 0");
            }
            self->setSpeed(speed);
            return 0;
        }

        int trackIsPlaying(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:isPlaying");
            push(L, self->isPlaying());
            return 1;
        }

        int trackIsPaused(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:isPaused");
            push(L, self->isPaused());
            return 1;
        }

        int trackSpeed(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:speed");
            push(L, self->speed());
            return 1;
        }

        int trackDuration(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:duration");
            push(L, self->duration());
            return 1;
        }

        int rigLoadAnimation(lua_State* L) {
            auto* rig = Usertype<LunarRig>::check(L, 1, "LunarRig:loadAnimation");
            CompiledAnimation compiled;
            if (auto* def = Usertype<LunarAnimationDef>::tryCheck(L, 2)) {
                auto result = compileAnimation(def->keyframes(), def->fps(), def->looped());
                if (result.isErr()) {
                    luaL_error(L, "LunarRig:loadAnimation: %s", result.unwrapErr().c_str());
                }
                compiled = std::move(result).unwrap();
            }
            else if (lua_istable(L, 2)) {
                auto parsed = parseAnimTable(L, 2, "LunarRig:loadAnimation");
                if (parsed.isErr()) {
                    luaL_error(L, "LunarRig:loadAnimation: %s", parsed.unwrapErr().c_str());
                }
                auto value = std::move(parsed).unwrap();
                auto result = compileAnimation(value.keyframes, value.fps, value.looped);
                if (result.isErr()) {
                    luaL_error(L, "LunarRig:loadAnimation: %s", result.unwrapErr().c_str());
                }
                compiled = std::move(result).unwrap();
            }
            else {
                luaL_error(L, "LunarRig:loadAnimation expected an animation or table at arg 2");
            }
            Usertype<LunarTrack>::pushOwned(L, LunarTrack::create(rig, std::move(compiled)));
            return 1;
        }

    } // namespace

    LunarAnimationDef* LunarAnimationDef::create() {
        auto* ret = new LunarAnimationDef();
        ret->autorelease();
        return ret;
    }

    void LunarAnimationDef::addKeyframe(double frame, std::string_view nodeId, NodePose pose) {
        setPoseTarget(keyframeFor(m_keyframes, frame), nodeId, std::move(pose));
    }

    LunarTrack* LunarTrack::create(LunarRig* rig, CompiledAnimation anim) {
        static std::atomic<int> s_nextTag{1};
        auto* ret = new LunarTrack();
        ret->m_rig = rig;
        ret->m_anim = std::move(anim);
        ret->m_tag = s_nextTag.fetch_add(1);
        ret->autorelease();
        liveTracks().push_back(ret);
        return ret;
    }

    LunarTrack::~LunarTrack() {
        if (Runtime::isShuttingDown()) return;
        stopActions();
        std::erase(liveTracks(), this);
    }

    void LunarTrack::launch(double fromTime) {
        stopActions();
        m_sliced = sliceAnimation(m_anim, fromTime);
        m_launchBase = fromTime;
        m_elapsed = 0.0;
        m_instants.clear();
        m_launched.clear();

        for (auto const& nodeTrack : m_sliced.nodes) {
            auto* node = m_rig ? m_rig->getNode(nodeTrack.nodeId) : nullptr;
            if (!node) {
                geode::log::warn(
                    "animation references missing rig node '{}', skipping", nodeTrack.nodeId
                );
                continue;
            }
            m_launched.push_back(node);

            TargetInstants instants;
            instants.node = node;
            for (auto const& seg : nodeTrack.segs) {
                if (!seg.instant) continue;
                instants.sets.emplace_back(std::max(0.0, seg.start - fromTime), seg.prop, seg.to);
            }
            std::sort(instants.sets.begin(), instants.sets.end(), [](auto const& a, auto const& b) {
                return std::get<0>(a) < std::get<0>(b);
            });
            if (!instants.sets.empty()) m_instants.push_back(std::move(instants));

            constexpr std::array<Prop, 6> kTweenProps = {
                Prop::PosX,
                Prop::PosY,
                Prop::Rotation,
                Prop::ScaleX,
                Prop::ScaleY,
                Prop::Opacity,
            };
            for (auto const prop : kTweenProps) {
                auto* actions = cocos2d::CCArray::create();
                for (auto const& seg : nodeTrack.segs) {
                    if (seg.instant || seg.prop != prop) continue;
                    double const delay = seg.start - fromTime;
                    if (delay > kTimeEps) {
                        actions->addObject(cocos2d::CCDelayTime::create(static_cast<float>(delay)));
                    }
                    if (auto* tween = makeTween(seg, node)) {
                        actions->addObject(tween);
                    }
                }
                if (actions->count() == 0) continue;
                auto* sequence = cocos2d::CCSequence::create(actions);
                sequence->setTag(m_tag);
                node->runAction(sequence);
            }
        }
        applyDueInstants();
    }

    void LunarTrack::stopActions() {
        for (auto const& node : m_launched) {
            node->stopActionByTag(m_tag);
        }
    }

    void LunarTrack::applyDueInstants() {
        for (auto& entry : m_instants) {
            while (entry.cursor < entry.sets.size() &&
                   std::get<0>(entry.sets[entry.cursor]) <= m_elapsed + kTimeEps) {
                applyInstant(
                    entry.node,
                    std::get<1>(entry.sets[entry.cursor]),
                    std::get<2>(entry.sets[entry.cursor])
                );
                ++entry.cursor;
            }
        }
    }

    void LunarTrack::finish() {
        stopActions();
        m_playing = false;
        m_paused = false;
        m_elapsed = m_sliced.duration;
    }

    void LunarTrack::play() {
        if (!m_rig) return;
        if (m_anim.nodes.empty() || !(m_anim.duration > 0.0)) {
            geode::log::warn("cannot play an animation without keyframes");
            return;
        }
        ensureTicker();
        m_playing = true;
        m_paused = false;
        launch(0.0);
    }

    void LunarTrack::pause() {
        if (!m_playing || m_paused) return;
        m_paused = true;
        for (auto const& node : m_launched) {
            if (auto* manager = node->getActionManager()) manager->pauseTarget(node);
        }
    }

    void LunarTrack::continuePlayback() {
        if (!m_playing || !m_paused) return;
        m_paused = false;
        for (auto const& node : m_launched) {
            if (auto* manager = node->getActionManager()) manager->resumeTarget(node);
        }
    }

    void LunarTrack::stop() {
        if (!m_playing) return;
        stopActions();
        m_playing = false;
        m_paused = false;
        m_elapsed = 0.0;
        m_launchBase = 0.0;
    }

    void LunarTrack::setSpeed(float speed) {
        m_speed = speed;
        if (!m_playing || m_paused) return;
        double playhead = m_launchBase + m_elapsed;
        if (m_sliced.looped && m_anim.duration > 0.0) {
            playhead = std::fmod(playhead, m_anim.duration);
        }
        if (!m_sliced.looped && playhead >= m_anim.duration) return;
        launch(playhead);
    }

    void LunarTrack::tick(float dt) {
        if (!m_playing || m_paused) return;
        m_elapsed += dt;
        applyDueInstants();
        if (m_elapsed + kTimeEps < m_sliced.duration) return;
        if (m_sliced.looped) {
            m_elapsed = std::fmod(m_elapsed, m_sliced.duration);
            launch(0.0);
        }
        else {
            finish();
        }
    }

    double LunarTrack::duration() const {
        return m_anim.duration;
    }

    void LunarTrack::detachForShutdown() {
        m_launched.clear();
        m_instants.clear();
        m_playing = false;
        m_paused = false;
    }

    void shutdownLunarTracks() {
        for (auto* track : liveTracks()) {
            if (track) track->detachForShutdown();
        }
        liveTracks().clear();
        if (auto*& node = tickNode()) {
            cocos2d::CCDirector::sharedDirector()->getScheduler()->unscheduleUpdateForTarget(node);
            node->release();
            node = nullptr;
        }
    }

    geode::Result<void> registerLunarAnimation(lua_State* L) {
        auto defResult = Usertype<LunarAnimationDef>::registerType(L, "LunarAnimationDef");
        if (defResult.isErr()) return defResult;
        auto trackResult = Usertype<LunarTrack>::registerType(L, "LunarAnimationTrack");
        if (trackResult.isErr()) return trackResult;

        Usertype<LunarAnimationDef>::method(L, "setFps", &animSetFps);
        Usertype<LunarAnimationDef>::method(L, "getFps", &animGetFps);
        Usertype<LunarAnimationDef>::method(L, "setLooped", &animSetLooped);
        Usertype<LunarAnimationDef>::method(L, "getLooped", &animGetLooped);
        Usertype<LunarAnimationDef>::method(L, "addKeyframe", &animAddKeyframe);

        Usertype<LunarRig>::method(L, "loadAnimation", &rigLoadAnimation);

        Usertype<LunarTrack>::method(L, "play", &trackPlay);
        Usertype<LunarTrack>::method(L, "pause", &trackPause);
        Usertype<LunarTrack>::method(L, "continue", &trackContinue);
        Usertype<LunarTrack>::method(L, "stop", &trackStop);
        Usertype<LunarTrack>::method(L, "setSpeed", &trackSetSpeed);
        Usertype<LunarTrack>::method(L, "isPlaying", &trackIsPlaying);
        Usertype<LunarTrack>::method(L, "isPaused", &trackIsPaused);
        Usertype<LunarTrack>::method(L, "speed", &trackSpeed);
        Usertype<LunarTrack>::method(L, "duration", &trackDuration);

        getOrCreateTable(L, "lunar.animation");
        setTableCFunction(L, -1, "new", &animNew);
        setTableCFunction(L, -1, "load", &animLoad);
        lua_pop(L, 1);

        return geode::Ok();
    }

} // namespace luax::lunar
