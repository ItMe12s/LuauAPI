#pragma once

#include <Geode/Result.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace luax::lunar {

    constexpr double kTimeEps = 1e-9;

    inline constexpr std::uint8_t opacityByte(float v) noexcept {
        return static_cast<std::uint8_t>(std::clamp(v, 0.F, 255.F));
    }

    enum class Prop : std::uint8_t {
        PosX,
        PosY,
        Rotation,
        ScaleX,
        ScaleY,
        Opacity,
        ZOrder,
    };

    static_assert(static_cast<int>(Prop::ZOrder) == 6);

    inline constexpr std::array<Prop, 7> kProps{
        Prop::PosX,
        Prop::PosY,
        Prop::Rotation,
        Prop::ScaleX,
        Prop::ScaleY,
        Prop::Opacity,
        Prop::ZOrder,
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

    struct EasingEntry {
        std::string_view name;
        EasingKind kind;
        float rate;
    };

    inline constexpr std::array<EasingEntry, 28> kEasingNames{{
        {"linear", EasingKind::Linear, 1.F},
        {"quad_in", EasingKind::PowIn, 2.F},
        {"quad_out", EasingKind::PowOut, 2.F},
        {"quad_in_out", EasingKind::PowInOut, 2.F},
        {"cubic_in", EasingKind::PowIn, 3.F},
        {"cubic_out", EasingKind::PowOut, 3.F},
        {"cubic_in_out", EasingKind::PowInOut, 3.F},
        {"quart_in", EasingKind::PowIn, 4.F},
        {"quart_out", EasingKind::PowOut, 4.F},
        {"quart_in_out", EasingKind::PowInOut, 4.F},
        {"quint_in", EasingKind::PowIn, 5.F},
        {"quint_out", EasingKind::PowOut, 5.F},
        {"quint_in_out", EasingKind::PowInOut, 5.F},
        {"sine_in", EasingKind::SineIn, 1.F},
        {"sine_out", EasingKind::SineOut, 1.F},
        {"sine_in_out", EasingKind::SineInOut, 1.F},
        {"expo_in", EasingKind::ExpoIn, 1.F},
        {"expo_out", EasingKind::ExpoOut, 1.F},
        {"expo_in_out", EasingKind::ExpoInOut, 1.F},
        {"back_in", EasingKind::BackIn, 1.F},
        {"back_out", EasingKind::BackOut, 1.F},
        {"back_in_out", EasingKind::BackInOut, 1.F},
        {"elastic_in", EasingKind::ElasticIn, 1.F},
        {"elastic_out", EasingKind::ElasticOut, 1.F},
        {"elastic_in_out", EasingKind::ElasticInOut, 1.F},
        {"bounce_in", EasingKind::BounceIn, 1.F},
        {"bounce_out", EasingKind::BounceOut, 1.F},
        {"bounce_in_out", EasingKind::BounceInOut, 1.F},
    }};

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
        std::vector<std::string> events;
    };

    struct AnimEvent {
        double time = 0.0;
        std::string name;
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
        std::vector<AnimEvent> events;
    };

    geode::Result<CompiledAnimation> compileAnimation(
        std::vector<Keyframe> keyframes, double fps, bool looped
    );

    CompiledAnimation sliceAnimation(CompiledAnimation const& src, double fromTime);

} // namespace luax::lunar
