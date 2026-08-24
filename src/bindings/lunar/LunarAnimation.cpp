#include "bindings/lunar/LunarAnimation.hpp"

#include "bindings/lunar/LunarCCAction.hpp"
#include "bindings/lunar/LunarRig.hpp"
#include "core/Config.hpp"
#include "core/Runtime.hpp"
#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/usertype/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <fmt/format.h>
#include <lua.h>
#include <lualib.h>

namespace luax::lunar {
    namespace {

        geode::Result<NodePose> parseNodePose(lua_State* L, int idx, std::string_view nodeId) {
            idx = lua_absindex(L, idx);
            luaL_checktype(L, idx, LUA_TTABLE);
            LuaStackGuard const guard(L);
            NodePose pose;
            lua_pushnil(L);
            while (lua_next(L, idx) != 0) {
                if (lua_type(L, -2) != LUA_TSTRING) {
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
                    if (!std::isfinite(value)) {
                        return geode::Err(
                            fmt::format("property '{}' must be finite (node '{}')", name, nodeId)
                        );
                    }
                    if (name == "x") pose.x = value;
                    else if (name == "y") pose.y = value;
                    else if (name == "rot") pose.rot = value;
                    else if (name == "sx") pose.sx = value;
                    else if (name == "sy") pose.sy = value;
                    else if (name == "opacity") pose.opacity = value;
                    else if (name == "z") pose.z = value;
                    else if (name == "ax") pose.ax = value;
                    else if (name == "ay") pose.ay = value;
                    else if (name == "skx") pose.skx = value;
                    else if (name == "sky") pose.sky = value;
                    else {
                        geode::log::warn("ignoring unknown animation property '{}'", name);
                    }
                }
                else if (name == "easing") {
                    if (lua_type(L, -1) != LUA_TSTRING) {
                        return geode::Err(fmt::format("'easing' must be a string (node '{}')", nodeId));
                    }
                    char const* easingName = lua_tostring(L, -1);
                    auto easing = easingFromString(easingName ? easingName : "");
                    if (!easing) {
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

        geode::Result<void> parseKeyframeEvents(
            lua_State* L, int idx, double frame, LunarAnimationDef* out
        ) {
            idx = lua_absindex(L, idx);
            if (lua_type(L, idx) == LUA_TSTRING) {
                size_t len = 0;
                char const* name = lua_tolstring(L, idx, &len);
                out->addEvent(frame, std::string(name ? name : "", len));
                return geode::Ok();
            }
            if (!lua_istable(L, idx)) {
                return geode::Err(std::string("'events' must be a string or an array of strings"));
            }
            lua_pushnil(L);
            while (lua_next(L, idx) != 0) {
                if (!lua_isnumber(L, -2)) {
                    return geode::Err(std::string("'events' must be an array of strings"));
                }
                if (lua_type(L, -1) != LUA_TSTRING) {
                    return geode::Err(
                        fmt::format("'events'[{}] must be a string", lua_tonumber(L, -2))
                    );
                }
                size_t len = 0;
                char const* name = lua_tolstring(L, -1, &len);
                out->addEvent(frame, std::string(name ? name : "", len));
                lua_pop(L, 1);
            }
            return geode::Ok();
        }

        cocos2d::CCFiniteTimeAction* makeTween(TweenSeg const& seg) {
            float const dur = static_cast<float>(std::max(0.0, seg.end - seg.start));
            using P = Prop;
            cocos2d::CCActionInterval* to = nullptr;
            switch (seg.prop) {
                case P::Rotation: to = cocos2d::CCRotateTo::create(dur, seg.to); break;
                case P::Opacity: to = cocos2d::CCFadeTo::create(dur, opacityByte(seg.to)); break;
                case P::ZOrder: break; // Instant-only, never reaches here.
                default: to = LunarCCAxisTo::create(dur, seg.prop, seg.to); break;
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
                case P::Opacity: setNodeOpacity(node, value); break;
                case P::ZOrder: node->setZOrder(static_cast<int>(value)); break;
                case P::AnchorX: node->setAnchorPoint({value, node->getAnchorPoint().y}); break;
                case P::AnchorY: node->setAnchorPoint({node->getAnchorPoint().x, value}); break;
                case P::SkewX: node->setSkewX(value); break;
                case P::SkewY: node->setSkewY(value); break;
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
            auto* director = cocos2d::CCDirector::sharedDirector();
            auto* scheduler = director ? director->getScheduler() : nullptr;
            if (!scheduler) {
                geode::log::warn("no scheduler available, animation ticker not armed");
                return;
            }
            auto* node = new LunarTickNode();
            node->retain();
            tickNode() = node;
            scheduler->scheduleUpdateForTarget(node, 0, false);
        }

        int animNew(lua_State* L) {
            Usertype<LunarAnimationDef>::pushOwned(L, LunarAnimationDef::create());
            return 1;
        }

        int animLoad(lua_State* L) {
            auto parsed = parseAnimTable(L, 1, "animation.load");
            if (auto err = returnIfErr(L, parsed)) return *err;
            Usertype<LunarAnimationDef>::pushOwned(L, std::move(parsed).unwrap());
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

        int animAddEvent(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:addEvent");
            double const frame = check<double>(L, 2, "LunarAnimationDef:addEvent");
            if (!(frame >= 0.0)) {
                luaL_error(L, "LunarAnimationDef:addEvent expected frame >= 0");
            }
            auto const name = check<std::string>(L, 3, "LunarAnimationDef:addEvent");
            self->addEvent(frame, name);
            return 0;
        }

        void pushPoseTable(lua_State* L, NodePose const& pose) {
            lua_createtable(L, 0, 12);
            auto set = [&](char const* key, std::optional<float> const& value) {
                if (!value) return;
                lua_pushnumber(L, static_cast<lua_Number>(*value));
                lua_setfield(L, -2, key);
            };
            set("x", pose.x);
            set("y", pose.y);
            set("rot", pose.rot);
            set("sx", pose.sx);
            set("sy", pose.sy);
            set("opacity", pose.opacity);
            set("z", pose.z);
            set("ax", pose.ax);
            set("ay", pose.ay);
            set("skx", pose.skx);
            set("sky", pose.sky);
            if (pose.easing.kind != EasingKind::Linear) {
                push(L, std::string(easingName(pose.easing)));
                lua_setfield(L, -2, "easing");
            }
        }

        void pushTargetsTable(lua_State* L, Keyframe const& kf) {
            lua_createtable(L, 0, static_cast<int>(kf.targets.size()));
            for (auto const& [id, pose] : kf.targets) {
                pushPoseTable(L, pose);
                lua_setfield(L, -2, id.c_str());
            }
        }

        void pushEventsTable(lua_State* L, std::vector<std::string> const& events) {
            lua_createtable(L, static_cast<int>(events.size()), 0);
            for (std::size_t e = 0; e < events.size(); ++e) {
                push(L, events[e]);
                lua_rawseti(L, -2, static_cast<int>(e) + 1);
            }
        }

        void pushKeyframeEntry(lua_State* L, Keyframe const& kf) {
            lua_createtable(L, 0, 2);
            pushTargetsTable(L, kf);
            lua_setfield(L, -2, "targets");
            pushEventsTable(L, kf.events);
            lua_setfield(L, -2, "events");
        }

        int animListKeyframes(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:listKeyframes");
            auto const& keyframes = self->keyframes();
            lua_createtable(L, static_cast<int>(keyframes.size()), 0);
            for (std::size_t i = 0; i < keyframes.size(); ++i) {
                auto const& kf = keyframes[i];
                lua_createtable(L, 0, 3);
                lua_pushnumber(L, static_cast<lua_Number>(kf.frame));
                lua_setfield(L, -2, "frame");
                pushTargetsTable(L, kf);
                lua_setfield(L, -2, "targets");
                pushEventsTable(L, kf.events);
                lua_setfield(L, -2, "events");
                lua_rawseti(L, -2, static_cast<int>(i) + 1);
            }
            return 1;
        }

        int animGetKeyAt(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:getKeyAt");
            double const frame = check<double>(L, 2, "LunarAnimationDef:getKeyAt");
            auto const& keyframes = self->keyframes();
            auto it = std::ranges::lower_bound(keyframes, frame, {}, &Keyframe::frame);
            if (it == keyframes.end() || !(std::fabs(it->frame - frame) < kTimeEps)) {
                lua_pushnil(L);
                return 1;
            }
            pushKeyframeEntry(L, *it);
            return 1;
        }

        int animRemoveKeyframe(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:removeKeyframe");
            double const frame = check<double>(L, 2, "LunarAnimationDef:removeKeyframe");
            if (!(frame >= 0.0)) {
                luaL_error(L, "LunarAnimationDef:removeKeyframe expected frame >= 0");
            }
            push(L, self->removeKeyframe(frame));
            return 1;
        }

        int animMoveKeyframe(lua_State* L) {
            auto* self = Usertype<LunarAnimationDef>::check(L, 1, "LunarAnimationDef:moveKeyframe");
            double const from = check<double>(L, 2, "LunarAnimationDef:moveKeyframe");
            double const to = check<double>(L, 3, "LunarAnimationDef:moveKeyframe");
            if (!(from >= 0.0 && to >= 0.0)) {
                luaL_error(L, "LunarAnimationDef:moveKeyframe expected frames >= 0");
            }
            push(L, self->moveKeyframe(from, to));
            return 1;
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

        int trackUnpause(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:unpause");
            self->unpause();
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

        int trackBindEvent(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:bindEvent");
            auto const name = check<std::string>(L, 2, "LunarAnimationTrack:bindEvent");
            luaL_checktype(L, 3, LUA_TFUNCTION);
            self->bindEvent(name, LuaCallback{L, 3});
            return 0;
        }

        int trackSample(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:sample");
            double const time = check<double>(L, 2, "LunarAnimationTrack:sample");
            auto poses = self->sample(time);
            lua_createtable(L, 0, static_cast<int>(poses.size()));
            for (auto const& [id, pose] : poses) {
                pushPoseTable(L, pose);
                lua_setfield(L, -2, id.c_str());
            }
            return 1;
        }

        int trackSeek(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:seek");
            double const time = check<double>(L, 2, "LunarAnimationTrack:seek");
            self->seek(time);
            return 0;
        }

        int trackCurrentTime(lua_State* L) {
            auto* self = Usertype<LunarTrack>::check(L, 1, "LunarAnimationTrack:currentTime");
            push(L, self->currentTime());
            return 1;
        }

    } // namespace

    geode::Result<LunarAnimationDef*> parseAnimTable(lua_State* L, int idx, char const* method) {
        luaL_checktype(L, idx, LUA_TTABLE);
        LuaStackGuard const guard(L);
        auto* out = LunarAnimationDef::create();

        if (auto fps = optNumberField(L, idx, "fps", method)) {
            if (!(fps > 0.0)) return geode::Err(std::string("'fps' must be > 0"));
            out->setFps(*fps);
        }
        if (auto looped = optBoolField(L, idx, "looped", method)) out->setLooped(*looped);

        lua_getfield(L, idx, "keyframes");
        if (!lua_istable(L, -1)) {
            return geode::Err(std::string("'keyframes' table is required"));
        }
        int const kfsIdx = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, kfsIdx) != 0) {
            if (!lua_isnumber(L, -2)) {
                return geode::Err(std::string("'keyframes' must be keyed by frame number"));
            }
            double const keyedFrame = lua_tonumber(L, -2);
            if (keyedFrame < 0.0 || !std::isfinite(keyedFrame)) {
                return geode::Err(
                    fmt::format("keyframe frame numbers must be >= 0 (got {})", keyedFrame)
                );
            }
            if (!lua_istable(L, -1)) {
                return geode::Err(fmt::format("keyframes[{}] must be a table", keyedFrame));
            }
            int const entry = lua_gettop(L);

            lua_pushnil(L);
            while (lua_next(L, entry) != 0) {
                if (lua_type(L, -2) == LUA_TSTRING &&
                    std::string_view(lua_tostring(L, -2)) == "frame") {
                    return geode::Err(
                        fmt::format("keyframes[{}] has a 'frame' key, use the table key as the frame number", keyedFrame)
                    );
                }
                if (lua_type(L, -2) == LUA_TSTRING &&
                    std::string_view(lua_tostring(L, -2)) == "events") {
                    auto events = parseKeyframeEvents(L, -1, keyedFrame, out);
                    if (events.isErr()) {
                        return geode::Err(
                            fmt::format("keyframes[{}]: {}", keyedFrame, events.unwrapErr())
                        );
                    }
                    lua_pop(L, 1);
                    continue;
                }
                if (lua_type(L, -2) != LUA_TSTRING) {
                    return geode::Err(
                        fmt::format("keyframes[{}] entries must map node ids to tables", keyedFrame)
                    );
                }
                char const* nodeId = lua_tostring(L, -2);
                auto poseResult = parseNodePose(L, -1, nodeId ? nodeId : "");
                if (poseResult.isErr()) {
                    return geode::Err(poseResult.unwrapErr());
                }
                out->addKeyframe(keyedFrame, nodeId ? nodeId : "", std::move(poseResult).unwrap());
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
        return geode::Ok(out);
    }

    LunarAnimationDef* LunarAnimationDef::create() {
        auto* ret = new LunarAnimationDef();
        ret->autorelease();
        return ret;
    }

    bool setNodeOpacity(cocos2d::CCNode* node, float value) {
        if (auto* rgba = geode::cast::typeinfo_cast<cocos2d::CCRGBAProtocol*>(node)) {
            rgba->setOpacity(opacityByte(value));
            return true;
        }
        return false;
    }

    void LunarAnimationDef::addKeyframe(double frame, std::string_view nodeId, NodePose pose) {
        setPoseTarget(keyframeFor(m_keyframes, frame), nodeId, std::move(pose));
    }

    void LunarAnimationDef::addEvent(double frame, std::string name) {
        keyframeFor(m_keyframes, frame).events.push_back(std::move(name));
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
        if (fromTime <= 0.0) {
            m_active = &m_anim;
        }
        else {
            m_sliced = sliceAnimation(m_anim, fromTime);
            m_active = &m_sliced;
        }
        m_launchBase = fromTime;
        m_elapsed = 0.0;
        m_eventCursor = 0;
        m_instants.clear();
        m_launched.clear();

        for (auto const& nodeTrack : m_active->nodes) {
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
                instants.sets.push_back(
                    TimedSet{
                        .time = std::max(0.0, seg.end - fromTime), .prop = seg.prop, .value = seg.to
                    }
                );
            }
            std::ranges::sort(instants.sets, {}, &TimedSet::time);
            if (!instants.sets.empty()) m_instants.push_back(std::move(instants));

            for (auto const prop : kProps) {
                auto* actions = cocos2d::CCArray::create();
                double prevEnd = fromTime;
                for (auto const& seg : nodeTrack.segs) {
                    if (seg.instant || seg.prop != prop) continue;
                    if (seg.start - prevEnd > kTimeEps) {
                        actions->addObject(
                            cocos2d::CCDelayTime::create(static_cast<float>(seg.start - prevEnd))
                        );
                    }
                    if (auto* tween = makeTween(seg)) {
                        actions->addObject(tween);
                    }
                    prevEnd = seg.end;
                }
                if (actions->count() == 0) continue;
                auto* sequence = cocos2d::CCSequence::create(actions);
                auto* speed = LunarCCSpeed::create(sequence, m_speed);
                speed->setTag(m_tag);
                node->runAction(speed);
                m_tweens.push_back(speed);
            }
        }
        applyDueInstants();
    }

    void LunarTrack::stopActions() {
        for (auto const& node : m_launched) {
            // At most one chain per property channel is ever tagged.
            for (std::size_t i = 0; i < kProps.size() && node->getActionByTag(m_tag); ++i) {
                node->stopActionByTag(m_tag);
            }
        }
        m_tweens.clear();
    }

    void LunarTrack::applyDueInstants() {
        for (auto& entry : m_instants) {
            while (entry.cursor < entry.sets.size() &&
                   entry.sets[entry.cursor].time <= m_elapsed + kTimeEps) {
                applyInstant(entry.node, entry.sets[entry.cursor].prop, entry.sets[entry.cursor].value);
                ++entry.cursor;
            }
        }
    }

    void LunarTrack::applyDueEvents() {
        if (!m_active) return;
        std::vector<std::string> due;
        while (m_eventCursor < m_active->events.size() &&
               m_active->events[m_eventCursor].time <= m_elapsed + kTimeEps) {
            due.push_back(m_active->events[m_eventCursor].name);
            ++m_eventCursor;
        }
        // Handlers can bind more fns while a fire is in progress btw.
        auto binds = m_eventBinds;
        for (auto const& name : due) {
            for (auto const& bind : binds) {
                if (bind.name != name) continue;
                std::string arg = name;
                bind.callback.invoke(
                    1,
                    0,
                    "LunarAnimationTrack:event",
                    kHookScriptDeadlineMs,
                    +[](lua_State* L, void* ctx) {
                        push(L, *static_cast<std::string*>(ctx));
                    },
                    &arg
                );
            }
        }
    }

    void LunarTrack::finish() {
        stopActions();
        m_playing = false;
        m_paused = false;
        m_elapsed = m_active ? m_active->duration : 0.0;
    }

    void LunarTrack::play() {
        if (!m_rig) return;
        if (!(m_anim.duration > 0.0)) {
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
        stopActions();
    }

    void LunarTrack::unpause() {
        if (!m_playing || !m_paused) return;
        m_paused = false;
        double playhead = m_launchBase + m_elapsed;
        if (m_active && m_active->looped && m_anim.duration > 0.0) {
            playhead = std::fmod(playhead, m_anim.duration);
        }
        launch(playhead);
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
        for (auto const& tween : m_tweens) {
            if (tween) tween->setSpeed(speed);
        }
    }

    void LunarTrack::bindEvent(std::string name, LuaCallback callback) {
        m_eventBinds.push_back(EventBind{std::move(name), std::move(callback)});
    }

    void LunarTrack::tick(float dt) {
        if (!m_playing || m_paused) return;
        m_elapsed += dt * m_speed;
        applyDueInstants();
        applyDueEvents();
        if (!m_active || m_elapsed + kTimeEps < m_active->duration) return;
        if (m_active->looped) {
            m_elapsed = std::fmod(m_elapsed, m_active->duration);
            launch(0.0);
        }
        else {
            finish();
        }
    }

    double LunarTrack::duration() const {
        return m_anim.duration;
    }

    void LunarTrack::seek(double time) {
        double const target = std::clamp(time, 0.0, std::max(0.0, m_anim.duration));
        if (m_playing && !m_paused && m_rig) {
            launch(target);
            return;
        }
        // Idle (paused/stopped/never started): no actions, direct pose write.
        // Events stay unarmed, the next launch() re-slices strictly past target.
        stopActions();
        m_sliced = sliceAnimation(m_anim, target);
        m_active = &m_sliced;
        m_launchBase = target;
        m_elapsed = 0.0;
        applyPose(m_rig, samplePose(m_anim, target));
    }

    void LunarTrack::detachForShutdown() {
        m_launched.clear();
        m_tweens.clear();
        m_instants.clear();
        m_eventBinds.clear();
        m_playing = false;
        m_paused = false;
    }

    std::unordered_map<std::string, NodePose> LunarTrack::sample(double time) {
        auto poses = samplePose(m_anim, time);
        applyPose(m_rig, poses);
        return poses;
    }

    void applyPose(LunarRig* rig, std::unordered_map<std::string, NodePose> const& poses) {
        if (!rig) return;
        for (auto const& [id, pose] : poses) {
            auto* node = rig->getNode(id);
            if (!node) continue;
            auto apply = [&](std::optional<float> const& value, Prop prop) {
                if (value) applyInstant(node, prop, *value);
            };
            apply(pose.x, Prop::PosX);
            apply(pose.y, Prop::PosY);
            apply(pose.rot, Prop::Rotation);
            apply(pose.sx, Prop::ScaleX);
            apply(pose.sy, Prop::ScaleY);
            apply(pose.opacity, Prop::Opacity);
            apply(pose.z, Prop::ZOrder);
            apply(pose.ax, Prop::AnchorX);
            apply(pose.ay, Prop::AnchorY);
            apply(pose.skx, Prop::SkewX);
            apply(pose.sky, Prop::SkewY);
        }
    }

    void shutdownLunarTracks() {
        for (auto* track : liveTracks()) {
            if (track) track->detachForShutdown();
        }
        liveTracks().clear();
        if (auto*& node = tickNode()) {
            if (auto* director = cocos2d::CCDirector::sharedDirector()) {
                if (auto* scheduler = director->getScheduler()) {
                    scheduler->unscheduleUpdateForTarget(node);
                }
            }
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
        Usertype<LunarAnimationDef>::method(L, "addEvent", &animAddEvent);
        Usertype<LunarAnimationDef>::method(L, "listKeyframes", &animListKeyframes);
        Usertype<LunarAnimationDef>::method(L, "getKeyAt", &animGetKeyAt);
        Usertype<LunarAnimationDef>::method(L, "removeKeyframe", &animRemoveKeyframe);
        Usertype<LunarAnimationDef>::method(L, "moveKeyframe", &animMoveKeyframe);

        Usertype<LunarTrack>::method(L, "play", &trackPlay);
        Usertype<LunarTrack>::method(L, "pause", &trackPause);
        Usertype<LunarTrack>::method(L, "unpause", &trackUnpause);
        Usertype<LunarTrack>::method(L, "stop", &trackStop);
        Usertype<LunarTrack>::method(L, "setSpeed", &trackSetSpeed);
        Usertype<LunarTrack>::method(L, "bindEvent", &trackBindEvent);
        Usertype<LunarTrack>::method(L, "isPlaying", &trackIsPlaying);
        Usertype<LunarTrack>::method(L, "isPaused", &trackIsPaused);
        Usertype<LunarTrack>::method(L, "speed", &trackSpeed);
        Usertype<LunarTrack>::method(L, "duration", &trackDuration);
        Usertype<LunarTrack>::method(L, "sample", &trackSample);
        Usertype<LunarTrack>::method(L, "seek", &trackSeek);
        Usertype<LunarTrack>::method(L, "currentTime", &trackCurrentTime);

        getOrCreateTable(L, "lunar.animation");
        setTableCFunction(L, -1, "new", &animNew);
        setTableCFunction(L, -1, "load", &animLoad);
        lua_pop(L, 1);

        return geode::Ok();
    }

} // namespace luax::lunar
