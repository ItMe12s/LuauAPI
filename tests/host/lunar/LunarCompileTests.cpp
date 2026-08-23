#include "bindings/lunar/LunarModel.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {
    using namespace luax::lunar;
    using Catch::Approx;

    Keyframe kf(double frame, std::string nodeId, NodePose pose) {
        Keyframe out;
        out.frame = frame;
        out.targets.emplace_back(std::move(nodeId), std::move(pose));
        return out;
    }

    Keyframe eventKf(double frame, std::vector<std::string> names) {
        Keyframe out;
        out.frame = frame;
        out.events = std::move(names);
        return out;
    }

    TweenSeg const* findSeg(CompiledAnimation const& anim, std::string const& id, Prop prop, bool instant) {
        for (auto const& track : anim.nodes) {
            if (track.nodeId != id) continue;
            for (auto const& seg : track.segs) {
                if (seg.prop == prop && seg.instant == instant) return &seg;
            }
        }
        return nullptr;
    }
} // namespace

TEST_CASE("easingFromString parses known names and rejects unknown") {
    auto linear = easingFromString("linear");
    REQUIRE(linear);
    REQUIRE(linear->kind == EasingKind::Linear);

    auto quadIn = easingFromString("quad_in");
    REQUIRE(quadIn);
    REQUIRE(quadIn->kind == EasingKind::PowIn);
    REQUIRE(quadIn->rate == Approx(2.F));

    auto quintInOut = easingFromString("quint_in_out");
    REQUIRE(quintInOut);
    REQUIRE(quintInOut->kind == EasingKind::PowInOut);
    REQUIRE(quintInOut->rate == Approx(5.F));

    auto backOut = easingFromString("back_out");
    REQUIRE(backOut);
    REQUIRE(backOut->kind == EasingKind::BackOut);

    auto bounceOut = easingFromString("bounce_out");
    REQUIRE(bounceOut);
    REQUIRE(bounceOut->kind == EasingKind::BounceOut);

    REQUIRE_FALSE(easingFromString("nope"));
    REQUIRE_FALSE(easingFromString(""));
}

TEST_CASE("easeProgress endpoints hold for every easing") {
    for (auto const& entry : kEasingNames) {
        auto easing = easingFromString(entry.name);
        REQUIRE(easing);
        INFO("easing: " << entry.name);
        REQUIRE(easeProgress(*easing, 0.F) == Approx(0.F).margin(1e-5));
        REQUIRE(easeProgress(*easing, 1.F) == Approx(1.F).margin(1e-5));
    }
}

TEST_CASE("easeProgress preserves shapes") {
    auto linear = *easingFromString("linear");
    REQUIRE(easeProgress(linear, 0.25F) == Approx(0.25F));
    REQUIRE(easeProgress(linear, 0.75F) == Approx(0.75F));

    auto quadIn = *easingFromString("quad_in");
    REQUIRE(easeProgress(quadIn, 0.25F) == Approx(0.0625F));

    auto quadOut = *easingFromString("quad_out");
    REQUIRE(easeProgress(quadOut, 0.25F) == Approx(0.4375F));

    auto cubicOut = *easingFromString("cubic_out");
    REQUIRE(easeProgress(cubicOut, 0.5F) == Approx(0.875F));

    auto backOut = *easingFromString("back_out");
    REQUIRE(easeProgress(backOut, 0.8F) > 1.F);

    auto bounceOut = *easingFromString("bounce_out");
    REQUIRE(easeProgress(bounceOut, 0.5F) == Approx(0.765625F));
}

TEST_CASE("compileAnimation builds tween and snap segments") {
    std::vector<Keyframe> const keyframes = {
        kf(10,
           "arm",
           [] {
               NodePose pose;
               pose.x = 10.F;
               return pose;
           }()),
        kf(20, "arm", [] {
            NodePose pose;
            pose.x = 20.F;
            return pose;
        }()),
    };

    auto result = compileAnimation(keyframes, 10.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();

    REQUIRE(anim.looped == false);
    REQUIRE(anim.duration == Approx(2.0));
    REQUIRE(anim.nodes.size() == 1);
    REQUIRE(anim.nodes[0].nodeId == "arm");

    auto const* snap = findSeg(anim, "arm", Prop::PosX, true);
    REQUIRE(snap);
    REQUIRE(snap->start == Approx(1.0));
    REQUIRE(snap->end == Approx(1.0));
    REQUIRE(snap->to == Approx(10.F));

    auto const* tween = findSeg(anim, "arm", Prop::PosX, false);
    REQUIRE(tween);
    REQUIRE(tween->start == Approx(1.0));
    REQUIRE(tween->end == Approx(2.0));
    REQUIRE(tween->from == Approx(10.F));
    REQUIRE(tween->to == Approx(20.F));
}

TEST_CASE("compileAnimation last keyframe wins at a duplicate frame") {
    std::vector<Keyframe> const keyframes = {
        kf(5,
           "arm",
           [] {
               NodePose pose;
               pose.x = 5.F;
               return pose;
           }()),
        kf(5, "arm", [] {
            NodePose pose;
            pose.x = 9.F;
            return pose;
        }()),
    };

    auto result = compileAnimation(keyframes, 10.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();

    REQUIRE(anim.nodes.size() == 1);
    REQUIRE(anim.nodes[0].segs.size() == 1);
    REQUIRE(anim.nodes[0].segs[0].instant);
    REQUIRE(anim.nodes[0].segs[0].to == Approx(9.F));
    REQUIRE(anim.duration == Approx(0.5));
}

TEST_CASE("compileAnimation keeps channels independent") {
    NodePose kf1;
    kf1.x = 0.F;
    kf1.rot = 0.F;
    NodePose kf2;
    kf2.rot = 90.F;
    NodePose kf3;
    kf3.x = 100.F;
    kf3.rot = 180.F;

    std::vector<Keyframe> const keyframes = {
        kf(1, "arm", kf1),
        kf(2, "arm", kf2),
        kf(3, "arm", kf3),
    };

    auto result = compileAnimation(keyframes, 1.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();

    auto const* xTween = findSeg(anim, "arm", Prop::PosX, false);
    REQUIRE(xTween);
    REQUIRE(xTween->start == Approx(1.0));
    REQUIRE(xTween->end == Approx(3.0));
    REQUIRE(xTween->to == Approx(100.F));

    auto const* rot1 = findSeg(anim, "arm", Prop::Rotation, false);
    REQUIRE(rot1);
    REQUIRE(rot1->start == Approx(1.0));
    REQUIRE(rot1->end == Approx(2.0));
    REQUIRE(rot1->to == Approx(90.F));

    REQUIRE(anim.duration == Approx(3.0));
}

TEST_CASE("compileAnimation validates fps and frame numbers") {
    std::vector<Keyframe> const keyframes = {kf(1, "arm", [] {
        NodePose pose;
        pose.x = 0.F;
        return pose;
    }())};

    REQUIRE(compileAnimation(keyframes, 0.0, false).isErr());
    REQUIRE(compileAnimation(keyframes, -5.0, false).isErr());

    std::vector<Keyframe> const negative = {kf(-1, "arm", [] {
        NodePose pose;
        pose.x = 0.F;
        return pose;
    }())};
    REQUIRE(compileAnimation(negative, 10.0, false).isErr());
}

TEST_CASE("compileAnimation rejects non-finite pose values") {
    auto makePose = [](float x) {
        NodePose pose;
        pose.x = x;
        return pose;
    };

    std::vector<Keyframe> const nanX = {
        kf(1, "arm", makePose(std::numeric_limits<float>::quiet_NaN()))
    };
    REQUIRE(compileAnimation(nanX, 10.0, false).isErr());

    std::vector<Keyframe> const infOpacity = {kf(1, "arm", [] {
        NodePose pose;
        pose.opacity = std::numeric_limits<float>::infinity();
        return pose;
    }())};
    REQUIRE(compileAnimation(infOpacity, 10.0, false).isErr());
}

TEST_CASE("compileAnimation merges near-duplicate frames keeping the last") {
    std::vector<Keyframe> const keyframes = {
        kf(5.0,
           "arm",
           [] {
               NodePose pose;
               pose.x = 5.F;
               return pose;
           }()),
        kf(5.0 + 1e-10, "arm", [] {
            NodePose pose;
            pose.x = 9.F;
            return pose;
        }()),
    };

    auto result = compileAnimation(keyframes, 10.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();
    REQUIRE(anim.nodes.size() == 1);
    REQUIRE(anim.nodes[0].segs.size() == 1);
    REQUIRE(anim.nodes[0].segs[0].instant);
    REQUIRE(anim.nodes[0].segs[0].to == Approx(9.F));
}

TEST_CASE("compileAnimation propagates easing onto tween segments") {
    NodePose start;
    start.x = 0.F;
    NodePose finish;
    finish.x = 10.F;
    finish.easing = *easingFromString("back_in");

    std::vector<Keyframe> const keyframes = {kf(0, "arm", start), kf(2, "arm", finish)};

    auto result = compileAnimation(keyframes, 1.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();
    auto const* tween = findSeg(anim, "arm", Prop::PosX, false);
    REQUIRE(tween);
    REQUIRE(tween->easing.kind == EasingKind::BackIn);
}

TEST_CASE("compileAnimation applies every z keyframe instantly") {
    std::vector<Keyframe> const keyframes = {
        kf(0,
           "arm",
           [] {
               NodePose pose;
               pose.z = 1.F;
               return pose;
           }()),
        kf(12, "arm", [] {
            NodePose pose;
            pose.z = 5.F;
            return pose;
        }()),
    };

    auto result = compileAnimation(keyframes, 12.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();

    std::vector<TweenSeg const*> snaps;
    for (auto const& track : anim.nodes) {
        if (track.nodeId != "arm") continue;
        for (auto const& seg : track.segs) {
            INFO("prop kind: " << static_cast<int>(seg.prop));
            REQUIRE(seg.instant);
            snaps.push_back(&seg);
        }
    }

    REQUIRE(snaps.size() == 2);
    REQUIRE(snaps[0]->end == Approx(0.0));
    REQUIRE(snaps[0]->to == Approx(1.F));
    REQUIRE(snaps[1]->end == Approx(1.0));
    REQUIRE(snaps[1]->to == Approx(5.F));

    REQUIRE_FALSE(findSeg(anim, "arm", Prop::ZOrder, false));
}

TEST_CASE("sliceAnimation keeps an instant exactly at the cut point") {
    NodePose start;
    start.z = 5.F;

    std::vector<Keyframe> const keyframes = {kf(1, "arm", start)};

    auto result = compileAnimation(keyframes, 2.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();

    CompiledAnimation sliced = sliceAnimation(anim, 0.5);
    auto const* zSnap = findSeg(sliced, "arm", Prop::ZOrder, true);
    REQUIRE(zSnap);
    REQUIRE(zSnap->to == Approx(5.F));
}

TEST_CASE("sliceAnimation keeps pending z instants by their own key time") {
    NodePose low;
    low.z = 1.F;
    NodePose high;
    high.z = 9.F;

    std::vector<Keyframe> const keyframes = {kf(1, "arm", low), kf(3, "arm", high)};

    auto result = compileAnimation(keyframes, 2.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();

    SECTION("cut between two z keys keeps the upcoming flip") {
        CompiledAnimation sliced = sliceAnimation(anim, 0.8);
        auto const* zSnap = findSeg(sliced, "arm", Prop::ZOrder, true);
        REQUIRE(zSnap);
        REQUIRE(zSnap->to == Approx(9.F));
        REQUIRE(zSnap->end == Approx(1.5));
    }

    SECTION("cut exactly at a z key keeps it") {
        CompiledAnimation sliced = sliceAnimation(anim, 1.5);
        auto const* zSnap = findSeg(sliced, "arm", Prop::ZOrder, true);
        REQUIRE(zSnap);
        REQUIRE(zSnap->to == Approx(9.F));
    }

    SECTION("cut past every z key drops them all") {
        CompiledAnimation sliced = sliceAnimation(anim, 1.6);
        REQUIRE_FALSE(findSeg(sliced, "arm", Prop::ZOrder, true));
    }
}

TEST_CASE("sliceAnimation clips elapsed tweens continuously") {
    NodePose start;
    start.x = 0.F;
    NodePose mid;
    mid.z = 5.F;
    NodePose finish;
    finish.x = 10.F;
    finish.easing = *easingFromString("cubic_out");

    std::vector<Keyframe> const keyframes = {
        kf(0, "arm", start),
        kf(1, "arm", mid),
        kf(2, "arm", finish),
    };

    auto result = compileAnimation(keyframes, 2.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();
    REQUIRE(anim.duration == Approx(1.0));

    SECTION("zero fromTime passes through") {
        CompiledAnimation sliced = sliceAnimation(anim, 0.0);
        REQUIRE(sliced.duration == Approx(1.0));
        REQUIRE(sliced.nodes.size() == anim.nodes.size());
    }

    SECTION("midpoint slice interpolates the from value") {
        CompiledAnimation sliced = sliceAnimation(anim, 0.5);
        auto const* tween = findSeg(sliced, "arm", Prop::PosX, false);
        REQUIRE(tween);
        REQUIRE(tween->start == Approx(0.5));
        REQUIRE(tween->end == Approx(1.0));
        float const expected =
            std::lerp(0.F, 10.F, easeProgress(*easingFromString("cubic_out"), 0.5F));
        REQUIRE(tween->from == Approx(expected));
        REQUIRE(tween->to == Approx(10.F));
        REQUIRE(sliced.duration == Approx(0.5));

        auto const* zSnap = findSeg(sliced, "arm", Prop::ZOrder, true);
        REQUIRE(zSnap);
        REQUIRE(zSnap->to == Approx(5.F));
    }

    SECTION("slice past an instant drops it") {
        CompiledAnimation sliced = sliceAnimation(anim, 0.6);
        REQUIRE_FALSE(findSeg(sliced, "arm", Prop::ZOrder, true));

        auto const* tween = findSeg(sliced, "arm", Prop::PosX, false);
        REQUIRE(tween);
        float const expected =
            std::lerp(0.F, 10.F, easeProgress(*easingFromString("cubic_out"), 0.6F));
        REQUIRE(tween->from == Approx(expected));
    }
}

TEST_CASE("sliceAnimation propagates looped flag") {
    NodePose pose;
    pose.x = 1.F;
    std::vector<Keyframe> const keyframes = {kf(0, "arm", pose)};

    auto result = compileAnimation(keyframes, 10.0, true);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();
    REQUIRE(anim.looped);

    CompiledAnimation sliced = sliceAnimation(anim, 0.0);
    REQUIRE(sliced.looped);
}

TEST_CASE("compileAnimation collects events sorted and extends duration") {
    NodePose pose;
    pose.x = 0.F;

    std::vector<Keyframe> const keyframes = {
        eventKf(67, {"late"}),
        kf(10, "arm", pose),
        eventKf(6, {"b", "a"}),
        eventKf(6, {"c"}),
    };

    auto result = compileAnimation(keyframes, 10.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();

    REQUIRE(anim.events.size() == 4);
    REQUIRE(anim.events[0].time == Approx(0.6));
    REQUIRE(anim.events[0].name == "b");
    REQUIRE(anim.events[1].time == Approx(0.6));
    REQUIRE(anim.events[1].name == "a");
    REQUIRE(anim.events[2].time == Approx(0.6));
    REQUIRE(anim.events[2].name == "c");
    REQUIRE(anim.events[3].time == Approx(6.7));
    REQUIRE(anim.events[3].name == "late");

    REQUIRE(anim.duration == Approx(6.7));

    REQUIRE(anim.nodes.size() == 1);
}

TEST_CASE("sliceAnimation shifts and drops events past the cut") {
    NodePose pose;
    pose.x = 0.F;

    std::vector<Keyframe> const keyframes = {
        eventKf(0, {"start"}),
        kf(10, "arm", pose),
        eventKf(20, {"mid"}),
        eventKf(40, {"end"}),
    };

    auto result = compileAnimation(keyframes, 10.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();
    REQUIRE(anim.duration == Approx(4.0));

    SECTION("zero fromTime keeps everything") {
        CompiledAnimation sliced = sliceAnimation(anim, 0.0);
        REQUIRE(sliced.events.size() == 3);
        REQUIRE(sliced.duration == Approx(4.0));
    }

    SECTION("mid slice drops earlier events and shifts kept ones") {
        CompiledAnimation sliced = sliceAnimation(anim, 2.5);
        REQUIRE(sliced.events.size() == 1);
        REQUIRE(sliced.events[0].name == "end");
        REQUIRE(sliced.events[0].time == Approx(1.5));
        REQUIRE(sliced.duration == Approx(1.5));
    }

    SECTION("slice past everything leaves no events") {
        CompiledAnimation sliced = sliceAnimation(anim, 5.0);
        REQUIRE(sliced.events.empty());
    }
}

TEST_CASE("compileAnimation tweens skew channels") {
    NodePose start;
    start.skx = 0.F;
    NodePose finish;
    finish.skx = 20.F;
    finish.sky = -10.F;

    std::vector<Keyframe> const keyframes = {kf(0, "arm", start), kf(2, "arm", finish)};

    auto result = compileAnimation(keyframes, 1.0, false);
    REQUIRE(result.isOk());
    auto anim = std::move(result).unwrap();

    auto const* snap = findSeg(anim, "arm", Prop::SkewX, true);
    REQUIRE(snap);
    REQUIRE(snap->start == Approx(0.0));
    REQUIRE(snap->end == Approx(0.0));
    REQUIRE(snap->to == Approx(0.F));

    auto const* xTween = findSeg(anim, "arm", Prop::SkewX, false);
    REQUIRE(xTween);
    REQUIRE(xTween->from == Approx(0.F));
    REQUIRE(xTween->to == Approx(20.F));

    auto const* ySnap = findSeg(anim, "arm", Prop::SkewY, true);
    REQUIRE(ySnap);
    REQUIRE(ySnap->to == Approx(-10.F));
    REQUIRE_FALSE(findSeg(anim, "arm", Prop::SkewY, false));

    CompiledAnimation sliced = sliceAnimation(anim, 1.0);
    auto const* clipped = findSeg(sliced, "arm", Prop::SkewX, false);
    REQUIRE(clipped);
    REQUIRE(clipped->from == Approx(10.F));
    REQUIRE(clipped->start == Approx(1.0));
    REQUIRE(clipped->to == Approx(20.F));
}

TEST_CASE("compileAnimation rejects non-finite skew values") {
    NodePose nanSkx;
    nanSkx.skx = std::numeric_limits<float>::quiet_NaN();
    std::vector<Keyframe> const nanKeyframes = {kf(1, "arm", nanSkx)};
    REQUIRE(compileAnimation(nanKeyframes, 10.0, false).isErr());

    NodePose infSky;
    infSky.sky = std::numeric_limits<float>::infinity();
    std::vector<Keyframe> const infKeyframes = {kf(1, "arm", infSky)};
    REQUIRE(compileAnimation(infKeyframes, 10.0, false).isErr());
}
