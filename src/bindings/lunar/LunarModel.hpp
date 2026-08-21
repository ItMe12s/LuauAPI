#pragma once

#include <Geode/Result.hpp>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace luax::lunar {

    enum class Prop : std::uint8_t {
        PosX,
        PosY,
        Rotation,
        ScaleX,
        ScaleY,
        Opacity,
        ZOrder,
    };

    enum class EasingKind : std::uint8_t {
        Linear,
        PowIn,
        PowOut,
        PowInOut,
        SineIn,
        SineOut,
        SineInOut,
        ExpoIn,
        ExpoOut,
        ExpoInOut,
        BackIn,
        BackOut,
        BackInOut,
        ElasticIn,
        ElasticOut,
        ElasticInOut,
        BounceIn,
        BounceOut,
        BounceInOut,
    };

    struct Easing {
        EasingKind kind = EasingKind::Linear;
        float rate = 1.F; // Only meaningful for Pow*
    };

    std::optional<Easing> easingFromString(std::string_view name);

    float easeProgress(Easing const& easing, float p);

    struct NodePose {
        std::optional<float> x;
        std::optional<float> y;
        std::optional<float> rot;
        std::optional<float> sx;
        std::optional<float> sy;
        std::optional<float> opacity;
        std::optional<float> z;
        Easing easing{};
    };

    struct Keyframe {
        double frame = 0.0;
        std::vector<std::pair<std::string, NodePose>> targets;
    };

    struct RigNodeSpec {
        std::string id;
        std::optional<std::string> sprite;
        std::optional<std::string> parent;
        float x = 0.F;
        float y = 0.F;
        float rot = 0.F;
        float sx = 1.F;
        float sy = 1.F;
        float z = 0.F;
        std::optional<float> opacity;
    };

    struct RigSpec {
        std::vector<RigNodeSpec> nodes;
    };

    struct TweenSeg {
        Prop prop{};
        double start = 0.0;
        double end = 0.0;
        float from = 0.F;
        float to = 0.F;
        Easing easing{};
        bool instant = false;
    };

    struct NodeTrack {
        std::string nodeId;
        std::vector<TweenSeg> segs;
    };

    struct CompiledAnimation {
        double duration = 0.0;
        bool looped = false;
        std::vector<NodeTrack> nodes;

        bool empty() const noexcept {
            return nodes.empty();
        }
    };

    geode::Result<CompiledAnimation> compileAnimation(
        std::span<Keyframe const> keyframes, double fps, bool looped
    );

    CompiledAnimation sliceAnimation(CompiledAnimation const& src, double fromTime);

} // namespace luax::lunar
