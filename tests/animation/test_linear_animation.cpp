#include <gtest/gtest.h>

#include <cmath>

#include <fire_engine/animation/linear_animation.hpp>
#include <fire_engine/math/vec3.hpp>

using fire_engine::LinearAnimation;
using fire_engine::Mat4;
using fire_engine::Vec3;

TEST(LinearAnimation, DefaultConstructionHasNoKeyframes)
{
    LinearAnimation anim;
    EXPECT_TRUE(anim.rotationKeyframes().empty());
    EXPECT_TRUE(anim.translationKeyframes().empty());
    EXPECT_FLOAT_EQ(anim.duration(), 0.0f);
}

TEST(LinearAnimation, SetKeyframesAndDuration)
{
    LinearAnimation anim;
    std::vector<LinearAnimation::RotationKeyframe> kf = {
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.7071f, 0.0f, 0.7071f},
        {2.0f, 0.0f, 1.0f, 0.0f, 0.0f},
    };
    anim.rotationKeyframes(kf);
    EXPECT_EQ(anim.rotationKeyframes().size(), 3u);
    EXPECT_FLOAT_EQ(anim.duration(), 2.0f);
}

TEST(LinearAnimation, SampleEmptyReturnsIdentity)
{
    LinearAnimation anim;
    Mat4 result = anim.sample(0.5f);
    Mat4 identity = Mat4::identity();
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            EXPECT_FLOAT_EQ((result[r, c]), (identity[r, c]));
        }
    }
}

TEST(LinearAnimation, SampleSingleKeyframeReturnsConstant)
{
    LinearAnimation anim;
    // Identity quaternion
    anim.rotationKeyframes({{0.0f, 0.0f, 0.0f, 0.0f, 1.0f}});

    Mat4 result = anim.sample(5.0f);
    Mat4 identity = Mat4::identity();
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            EXPECT_NEAR((result[r, c]), (identity[r, c]), 1e-5f);
        }
    }
}

TEST(LinearAnimation, SampleAtExactKeyframeTime)
{
    LinearAnimation anim;
    // Keyframe at t=0: identity quaternion (no rotation)
    // Keyframe at t=1: 90 degrees around Y axis -> quat (0, sin(45), 0, cos(45))
    float s45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    float c45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {2.0f, 0.0f, s45, 0.0f, c45},
    });

    // Sample at t=0 should be identity
    Mat4 atZero = anim.sample(0.0f);
    EXPECT_NEAR((atZero[0, 0]), 1.0f, 1e-5f);
    EXPECT_NEAR((atZero[1, 1]), 1.0f, 1e-5f);
    EXPECT_NEAR((atZero[2, 2]), 1.0f, 1e-5f);

    // Sample near end should approach 90-degree Y rotation
    // For 90 deg Y rotation: m[0,0]=0, m[0,2]=1, m[2,0]=-1, m[2,2]=0
    Mat4 atEnd = anim.sample(1.999f);
    EXPECT_NEAR((atEnd[0, 0]), 0.0f, 2e-2f);
    EXPECT_NEAR((atEnd[0, 2]), 1.0f, 2e-2f);
    EXPECT_NEAR((atEnd[2, 0]), -1.0f, 2e-2f);
    EXPECT_NEAR((atEnd[2, 2]), 0.0f, 2e-2f);
    EXPECT_NEAR((atEnd[1, 1]), 1.0f, 2e-2f);
}

TEST(LinearAnimation, SampleInterpolatesBetweenKeyframes)
{
    LinearAnimation anim;
    float s45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    float c45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {2.0f, 0.0f, s45, 0.0f, c45},
    });

    // Sample at t=1 (midpoint) should be ~45 degrees around Y
    Mat4 mid = anim.sample(1.0f);
    // cos(45) ~= 0.7071
    float cos45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    float sin45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    EXPECT_NEAR((mid[0, 0]), cos45, 1e-4f);
    EXPECT_NEAR((mid[0, 2]), sin45, 1e-4f);
    EXPECT_NEAR((mid[2, 0]), -sin45, 1e-4f);
    EXPECT_NEAR((mid[2, 2]), cos45, 1e-4f);
}

TEST(LinearAnimation, SampleLoops)
{
    LinearAnimation anim;
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {2.0f, 0.0f, 1.0f, 0.0f, 0.0f},
    });

    // Sample at t=0 and t=2.0 (one full loop) should be the same
    Mat4 atZero = anim.sample(0.0f);
    Mat4 atLoop = anim.sample(2.0f);
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            EXPECT_NEAR((atZero[r, c]), (atLoop[r, c]), 1e-4f);
        }
    }
}

TEST(LinearAnimation, SampleAndDurationAreNoexcept)
{
    LinearAnimation anim;
    EXPECT_TRUE(noexcept(anim.sample(0.0f)));
    EXPECT_TRUE(noexcept(anim.duration()));
}

// ==========================================================================
// Translation sampling
// ==========================================================================

TEST(LinearAnimation, EmptyTranslationSamplesToZeroOffset)
{
    LinearAnimation anim;
    // Give it a single rotation keyframe so sample() doesn't short-circuit to identity
    anim.rotationKeyframes({{0.0f, 0.0f, 0.0f, 0.0f, 1.0f}});
    Mat4 m = anim.sample(0.5f);
    // Translation column should be zero
    EXPECT_NEAR((m[0, 3]), 0.0f, 1e-5f);
    EXPECT_NEAR((m[1, 3]), 0.0f, 1e-5f);
    EXPECT_NEAR((m[2, 3]), 0.0f, 1e-5f);
}

TEST(LinearAnimation, SingleTranslationKeyframeIsConstant)
{
    LinearAnimation anim;
    anim.translationKeyframes({{0.0f, Vec3{1.0f, 2.0f, 3.0f}}});
    Mat4 m = anim.sample(42.0f);
    EXPECT_NEAR((m[0, 3]), 1.0f, 1e-5f);
    EXPECT_NEAR((m[1, 3]), 2.0f, 1e-5f);
    EXPECT_NEAR((m[2, 3]), 3.0f, 1e-5f);
}

TEST(LinearAnimation, TranslationInterpolatesLinearly)
{
    LinearAnimation anim;
    anim.translationKeyframes({
        {0.0f, Vec3{0.0f, 0.0f, 0.0f}},
        {2.0f, Vec3{2.0f, 4.0f, 6.0f}},
    });
    Mat4 m = anim.sample(1.0f);
    EXPECT_NEAR((m[0, 3]), 1.0f, 1e-5f);
    EXPECT_NEAR((m[1, 3]), 2.0f, 1e-5f);
    EXPECT_NEAR((m[2, 3]), 3.0f, 1e-5f);
}

// ==========================================================================
// Duration = max across channels
// ==========================================================================

TEST(LinearAnimation, DurationIsMaxAcrossChannels)
{
    LinearAnimation anim;
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    });
    anim.translationKeyframes({
        {0.0f, Vec3{}},
        {2.0f, Vec3{1.0f, 0.0f, 0.0f}},
    });
    EXPECT_FLOAT_EQ(anim.duration(), 2.0f);
}

// ==========================================================================
// Composite T * R
// ==========================================================================

TEST(LinearAnimation, CompositeTranslationAndRotationMidpoint)
{
    LinearAnimation anim;

    // Rotation: 0° -> 90° about Y over 2s
    float s45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    float c45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {2.0f, 0.0f, s45, 0.0f, c45},
    });

    // Translation: (0,0,0) -> (2,0,0) over 2s
    anim.translationKeyframes({
        {0.0f, Vec3{0.0f, 0.0f, 0.0f}},
        {2.0f, Vec3{2.0f, 0.0f, 0.0f}},
    });

    // At t=1s: rotation is 45° about Y, translation is (1,0,0)
    Mat4 m = anim.sample(1.0f);

    // Rotation part (upper-left 3x3) should match 45° Y rotation
    EXPECT_NEAR((m[0, 0]), c45, 1e-4f);
    EXPECT_NEAR((m[0, 2]), s45, 1e-4f);
    EXPECT_NEAR((m[2, 0]), -s45, 1e-4f);
    EXPECT_NEAR((m[2, 2]), c45, 1e-4f);
    EXPECT_NEAR((m[1, 1]), 1.0f, 1e-4f);

    // Translation column should be (1, 0, 0) — T * R leaves translation in last column
    EXPECT_NEAR((m[0, 3]), 1.0f, 1e-4f);
    EXPECT_NEAR((m[1, 3]), 0.0f, 1e-4f);
    EXPECT_NEAR((m[2, 3]), 0.0f, 1e-4f);
}

// ==========================================================================
// Channels with different durations clamp independently
// ==========================================================================

TEST(LinearAnimation, ExplicitDurationOverridesComputedMax)
{
    LinearAnimation anim;
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    });
    // Computed max would be 1.0f — force it to 5.0f so this animation loops in
    // lockstep with a sibling that runs longer.
    anim.duration(5.0f);
    EXPECT_FLOAT_EQ(anim.duration(), 5.0f);
}

TEST(LinearAnimation, SampleLoopsOnExplicitDurationAndClampsShortChannel)
{
    LinearAnimation anim;
    // Rotation ends at t=1.0 — 0° to 90° about Y.
    float s45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    float c45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, s45, 0.0f, c45},
    });
    // Force duration to 2.0 — simulates a sibling channel running to t=2.0.
    anim.duration(2.0f);

    // At t=1.5 (past rotation's last key, still within shared duration), rotation
    // must clamp to last key (90° Y), NOT loop early.
    Mat4 m = anim.sample(1.5f);
    EXPECT_NEAR((m[0, 0]), 0.0f, 1e-4f);
    EXPECT_NEAR((m[0, 2]), 1.0f, 1e-4f);
    EXPECT_NEAR((m[2, 0]), -1.0f, 1e-4f);
    EXPECT_NEAR((m[2, 2]), 0.0f, 1e-4f);

    // At t=2.5 we wrap to 0.5 on the shared duration — half of rotation → 45° Y.
    Mat4 wrapped = anim.sample(2.5f);
    EXPECT_NEAR((wrapped[0, 0]), c45, 1e-4f);
    EXPECT_NEAR((wrapped[0, 2]), s45, 1e-4f);
}

TEST(LinearAnimation, RotationClampsToFirstKeyframeWhenSampledBeforeItsStart)
{
    LinearAnimation anim;
    // Rotation channel starts at t=1.0 with identity, goes to 90° about Y at t=2.0 —
    // mirrors BoxAnimated where the rotation window starts mid-animation.
    float s45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    float c45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    anim.rotationKeyframes({
        {1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {2.0f, 0.0f, s45, 0.0f, c45},
    });
    // Give it a translation so the animation's duration extends back to t=0.
    anim.translationKeyframes({
        {0.0f, Vec3{0.0f, 0.0f, 0.0f}},
        {2.0f, Vec3{0.0f, 0.0f, 0.0f}},
    });

    // At t=0.5 (before the rotation's first keyframe), the rotation must clamp
    // to the first keyframe (identity), NOT extrapolate backwards via slerp.
    Mat4 m = anim.sample(0.5f);

    EXPECT_NEAR((m[0, 0]), 1.0f, 1e-5f);
    EXPECT_NEAR((m[1, 1]), 1.0f, 1e-5f);
    EXPECT_NEAR((m[2, 2]), 1.0f, 1e-5f);
    EXPECT_NEAR((m[0, 2]), 0.0f, 1e-5f);
    EXPECT_NEAR((m[2, 0]), 0.0f, 1e-5f);
}

TEST(LinearAnimation, ShorterChannelClampsWhileLongerInterpolates)
{
    LinearAnimation anim;

    // Rotation ends at t=1: 0° -> 90° about Y
    float s45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    float c45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, s45, 0.0f, c45},
    });

    // Translation ends at t=2: (0,0,0) -> (2,0,0)
    anim.translationKeyframes({
        {0.0f, Vec3{0.0f, 0.0f, 0.0f}},
        {2.0f, Vec3{2.0f, 0.0f, 0.0f}},
    });

    // duration() = 2.0, so t=1.5 is in-range (no wrap)
    Mat4 m = anim.sample(1.5f);

    // Rotation should clamp to its last keyframe: 90° about Y
    // For 90°: m[0,0]=0, m[0,2]=1, m[2,0]=-1, m[2,2]=0
    EXPECT_NEAR((m[0, 0]), 0.0f, 1e-4f);
    EXPECT_NEAR((m[0, 2]), 1.0f, 1e-4f);
    EXPECT_NEAR((m[2, 0]), -1.0f, 1e-4f);
    EXPECT_NEAR((m[2, 2]), 0.0f, 1e-4f);

    // Translation should be 75% of the way to (2,0,0) = (1.5, 0, 0)
    EXPECT_NEAR((m[0, 3]), 1.5f, 1e-4f);
    EXPECT_NEAR((m[1, 3]), 0.0f, 1e-4f);
    EXPECT_NEAR((m[2, 3]), 0.0f, 1e-4f);
}
