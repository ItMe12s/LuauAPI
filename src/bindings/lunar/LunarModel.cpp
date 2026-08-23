#include "bindings/lunar/LunarModel.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fmt/format.h>
#include <numbers>
#include <string_view>
#include <unordered_map>

namespace luax::lunar {
    namespace {

        constexpr float kPi = std::numbers::pi_v<float>;
        constexpr float kTwoPi = kPi * 2.F;

        constexpr float bounceOut(float p) {
            constexpr float n1 = 7.5625F;
            constexpr float d1 = 2.75F;
            if (p < 1.F / d1) return n1 * p * p;
            if (p < 2.F / d1) {
                p -= 1.5F / d1;
                return n1 * p * p + 0.75F;
            }
            if (p < 2.5F / d1) {
                p -= 2.25F / d1;
                return n1 * p * p + 0.9375F;
            }
            p -= 2.625F / d1;
            return n1 * p * p + 0.984375F;
        }

        constexpr bool sameTime(double a, double b) {
            return std::fabs(a - b) < kTimeEps;
        }

        struct ChannelKey {
            double time;
            float value;
            Easing easing;
        };

        using ChannelMap = std::array<std::vector<ChannelKey>, 11>;

        std::vector<TweenSeg> buildSegs(Prop prop, std::vector<ChannelKey>& keys) {
            std::ranges::sort(keys, {}, &ChannelKey::time);
            // Last one wins.
            std::vector<ChannelKey> deduped;
            for (auto it = keys.begin(); it != keys.end(); ++it) {
                auto next = std::next(it);
                if (next != keys.end() && sameTime(it->time, next->time)) continue;
                deduped.push_back(*it);
            }

            std::vector<TweenSeg> segs;
            if (deduped.empty()) return segs;
            segs.reserve(deduped.size());
            segs.push_back(
                TweenSeg{
                    .prop = prop,
                    .start = deduped.front().time,
                    .end = deduped.front().time,
                    .from = 0.F,
                    .to = deduped.front().value,
                    .easing = deduped.front().easing,
                    .instant = true
                }
            );
            for (std::size_t i = 1; i < deduped.size(); ++i) {
                auto const& prev = deduped[i - 1];
                auto const& cur = deduped[i];
                if (sameTime(prev.time, cur.time)) continue;
                segs.push_back(
                    TweenSeg{
                        .prop = prop,
                        .start = prev.time,
                        .end = cur.time,
                        .from = prev.value,
                        .to = cur.value,
                        .easing = cur.easing,
                        .instant = prop == Prop::ZOrder
                    }
                );
            }
            return segs;
        }

        std::optional<float> sampleChannel(std::vector<TweenSeg> const& segs, Prop prop, double t) {
            std::optional<float> value;
            for (auto const& seg : segs) {
                if (seg.prop != prop) continue;
                if (seg.start > t + kTimeEps) continue;
                if (seg.instant || t >= seg.end - kTimeEps) {
                    value = seg.to;
                    continue;
                }
                double const span = seg.end - seg.start;
                float const p = span > 0.0 ? static_cast<float>((t - seg.start) / span) : 1.F;
                value = std::lerp(seg.from, seg.to, easeProgress(seg.easing, p));
            }
            return value;
        }

    } // namespace

    std::optional<Easing> easingFromString(std::string_view name) {
        for (auto const& entry : kEasingNames) {
            if (entry.name == name) return Easing{entry.kind, entry.rate};
        }
        return std::nullopt;
    }

    float easeProgress(Easing const& easing, float p) {
        using K = EasingKind;
        p = std::clamp(p, 0.F, 1.F);
        switch (easing.kind) {
            case K::Linear: return p;
            case K::PowIn: return std::pow(p, easing.rate);
            case K::PowOut: return 1.F - std::pow(1.F - p, easing.rate);
            case K::PowInOut: {
                float t = p * 2.F;
                if (t < 1.F) return 0.5F * std::pow(t, easing.rate);
                return 1.F - 0.5F * std::pow(2.F - t, easing.rate);
            }
            case K::SineIn: return 1.F - std::cos(p * kPi * 0.5F);
            case K::SineOut: return std::sin(p * kPi * 0.5F);
            case K::SineInOut: return -0.5F * (std::cos(kPi * p) - 1.F);
            case K::ExpoIn:
                if (p <= 0.F) return 0.F;
                return std::pow(2.F, 10.F * p - 10.F);
            case K::ExpoOut:
                if (p >= 1.F) return 1.F;
                return 1.F - std::pow(2.F, -10.F * p);
            case K::ExpoInOut: {
                if (p <= 0.F) return 0.F;
                if (p >= 1.F) return 1.F;
                if (p < 0.5F) return 0.5F * std::pow(2.F, 20.F * p - 10.F);
                return (2.F - std::pow(2.F, -20.F * p + 10.F)) * 0.5F;
            }
            case K::BackIn: {
                constexpr float c1 = 1.70158F;
                constexpr float c3 = c1 + 1.F;
                return c3 * p * p * p - c1 * p * p;
            }
            case K::BackOut: {
                constexpr float c1 = 1.70158F;
                constexpr float c3 = c1 + 1.F;
                float t = p - 1.F;
                return 1.F + c3 * t * t * t + c1 * t * t;
            }
            case K::BackInOut: {
                constexpr float c2 = 1.70158F * 1.525F;
                if (p < 0.5F) {
                    float t = 2.F * p;
                    return (c2 + 1.F) * t * t * t - c2 * t * t;
                }
                float t = 2.F * p - 2.F;
                return 1.F + (c2 + 1.F) * t * t * t + c2 * t * t;
            }
            case K::ElasticIn: {
                if (p <= 0.F) return 0.F;
                if (p >= 1.F) return 1.F;
                constexpr float c4 = kTwoPi / 3.F;
                return -std::pow(2.F, 10.F * p - 10.F) * std::sin((p * 10.F - 10.75F) * c4);
            }
            case K::ElasticOut: {
                if (p <= 0.F) return 0.F;
                if (p >= 1.F) return 1.F;
                constexpr float c4 = kTwoPi / 3.F;
                return std::pow(2.F, -10.F * p) * std::sin((p * 10.F - 0.75F) * c4) + 1.F;
            }
            case K::ElasticInOut: {
                if (p <= 0.F) return 0.F;
                if (p >= 1.F) return 1.F;
                constexpr float c4 = kTwoPi / 4.5F;
                if (p < 0.5F) {
                    return -0.5F * std::pow(2.F, 20.F * p - 10.F) *
                        std::sin((20.F * p - 11.125F) * c4);
                }
                return std::pow(2.F, -20.F * p + 10.F) * std::sin((20.F * p - 11.125F) * c4) * 0.5F +
                    1.F;
            }
            case K::BounceIn: return 1.F - bounceOut(1.F - p);
            case K::BounceOut: return bounceOut(p);
            case K::BounceInOut: {
                if (p < 0.5F) return (1.F - bounceOut(1.F - 2.F * p)) * 0.5F;
                return (1.F + bounceOut(2.F * p - 1.F)) * 0.5F;
            }
        }
        return p;
    }

    geode::Result<CompiledAnimation> compileAnimation(
        std::vector<Keyframe> keyframes, double fps, bool looped
    ) {
        if (!(fps > 0.0) || !std::isfinite(fps)) {
            return geode::Err(std::string("fps must be a positive finite number"));
        }

        std::ranges::stable_sort(keyframes, {}, &Keyframe::frame);
        for (auto const& kf : keyframes) {
            if (kf.frame < 0.0 || !std::isfinite(kf.frame)) {
                return geode::Err(
                    fmt::format("keyframe frame numbers must be >= 0 (got {})", kf.frame)
                );
            }
        }

        for (auto const& kf : keyframes) {
            for (auto const& [nodeId, pose] : kf.targets) {
                auto finite = [](std::optional<float> const& v) {
                    return !v || std::isfinite(*v);
                };
                if (!finite(pose.x) || !finite(pose.y) || !finite(pose.rot) || !finite(pose.sx) ||
                    !finite(pose.sy) || !finite(pose.opacity) || !finite(pose.z) ||
                    !finite(pose.ax) || !finite(pose.ay) || !finite(pose.skx) || !finite(pose.sky)) {
                    return geode::Err(fmt::format("pose values for node '{}' must be finite", nodeId));
                }
            }
        }

        std::unordered_map<std::string, ChannelMap> store;
        for (auto const& kf : keyframes) {
            double const time = kf.frame / fps;
            for (auto const& [nodeId, pose] : kf.targets) {
                auto& channels = store[nodeId];
                auto push = [&](std::optional<float> const& value, Prop prop) {
                    if (!value) return;
                    channels[static_cast<std::size_t>(prop)].push_back({time, *value, pose.easing});
                };
                push(pose.x, Prop::PosX);
                push(pose.y, Prop::PosY);
                push(pose.rot, Prop::Rotation);
                push(pose.sx, Prop::ScaleX);
                push(pose.sy, Prop::ScaleY);
                push(pose.opacity, Prop::Opacity);
                push(pose.z, Prop::ZOrder);
                push(pose.ax, Prop::AnchorX);
                push(pose.ay, Prop::AnchorY);
                push(pose.skx, Prop::SkewX);
                push(pose.sky, Prop::SkewY);
            }
        }

        CompiledAnimation out;
        out.looped = looped;
        double duration = 0.0;
        out.nodes.reserve(store.size());
        for (auto& [nodeId, channels] : store) {
            NodeTrack track;
            track.nodeId = nodeId;
            std::size_t largestProp = 0;
            for (std::size_t i = 0; i < kProps.size(); ++i) {
                largestProp = std::max(largestProp, channels[i].size());
            }
            track.segs.reserve(largestProp * 2 + 1);
            for (std::size_t i = 0; i < kProps.size(); ++i) {
                if (channels[i].empty()) continue;
                auto segs = buildSegs(kProps[i], channels[i]);
                for (auto const& seg : segs) {
                    duration = std::max(duration, seg.end);
                }
                track.segs.insert(
                    track.segs.end(),
                    std::make_move_iterator(segs.begin()),
                    std::make_move_iterator(segs.end())
                );
            }
            if (!track.segs.empty()) {
                out.nodes.push_back(std::move(track));
            }
        }

        std::vector<AnimEvent> events;
        for (auto const& kf : keyframes) {
            double const time = kf.frame / fps;
            for (auto const& name : kf.events) {
                duration = std::max(duration, time);
                events.push_back(AnimEvent{time, name});
            }
        }
        std::ranges::stable_sort(events, {}, &AnimEvent::time);
        out.events = std::move(events);

        out.duration = duration;
        return geode::Ok(std::move(out));
    }

    CompiledAnimation sliceAnimation(CompiledAnimation const& src, double fromTime) {
        CompiledAnimation out;
        out.looped = src.looped;
        if (fromTime <= 0.0) {
            out.duration = src.duration;
            out.nodes = src.nodes;
            out.events = src.events;
            return out;
        }

        double duration = 0.0;
        out.nodes.reserve(src.nodes.size());
        for (auto const& node : src.nodes) {
            NodeTrack track;
            track.nodeId = node.nodeId;
            track.segs.reserve(node.segs.size());
            for (auto const& seg : node.segs) {
                if (seg.instant) {
                    if (seg.end >= fromTime) track.segs.push_back(seg);
                    continue;
                }
                if (seg.end <= fromTime) continue;
                TweenSeg kept = seg;
                if (seg.start < fromTime) {
                    double const span = seg.end - seg.start;
                    float const p =
                        span > 0.0 ? static_cast<float>((fromTime - seg.start) / span) : 1.F;
                    float const eased = easeProgress(seg.easing, p);
                    kept.from = std::lerp(seg.from, seg.to, eased);
                    kept.start = fromTime;
                }
                track.segs.push_back(kept);
            }
            for (auto const& seg : track.segs) {
                duration = std::max(duration, seg.end - fromTime);
            }
            if (!track.segs.empty()) out.nodes.push_back(std::move(track));
        }

        for (auto const& ev : src.events) {
            if (ev.time < fromTime) continue;
            double const shifted = ev.time - fromTime;
            duration = std::max(duration, shifted);
            out.events.push_back(AnimEvent{shifted, ev.name});
        }

        out.duration = std::max(0.0, duration);
        return out;
    }

    std::unordered_map<std::string, NodePose> samplePose(CompiledAnimation const& anim, double time) {
        std::unordered_map<std::string, NodePose> out;
        out.reserve(anim.nodes.size());
        for (auto const& track : anim.nodes) {
            NodePose pose;
            pose.x = sampleChannel(track.segs, Prop::PosX, time);
            pose.y = sampleChannel(track.segs, Prop::PosY, time);
            pose.rot = sampleChannel(track.segs, Prop::Rotation, time);
            pose.sx = sampleChannel(track.segs, Prop::ScaleX, time);
            pose.sy = sampleChannel(track.segs, Prop::ScaleY, time);
            pose.opacity = sampleChannel(track.segs, Prop::Opacity, time);
            pose.z = sampleChannel(track.segs, Prop::ZOrder, time);
            pose.ax = sampleChannel(track.segs, Prop::AnchorX, time);
            pose.ay = sampleChannel(track.segs, Prop::AnchorY, time);
            pose.skx = sampleChannel(track.segs, Prop::SkewX, time);
            pose.sky = sampleChannel(track.segs, Prop::SkewY, time);
            if (pose.opacity) {
                pose.opacity = static_cast<float>(opacityByte(*pose.opacity));
            }
            out.emplace(track.nodeId, std::move(pose));
        }
        return out;
    }

} // namespace luax::lunar
