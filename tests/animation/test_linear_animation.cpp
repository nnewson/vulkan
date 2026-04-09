#include <gtest/gtest.h>

#include <cmath>

#include <fire_engine/animation/linear_animation.hpp>

using fire_engine::LinearAnimation;
using fire_engine::Mat4;

TEST(LinearAnimation, DefaultConstructionHasNoKeyframes)
{
    LinearAnimation anim;
    EXPECT_TRUE(anim.keyframes().empty());
    EXPECT_FLOAT_EQ(anim.duration(), 0.0f);
}

TEST(LinearAnimation, SetKeyframesAndDuration)
{
    LinearAnimation anim;
    std::vector<LinearAnimation::Keyframe> kf = {
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.7071f, 0.0f, 0.7071f},
        {2.0f, 0.0f, 1.0f, 0.0f, 0.0f},
    };
    anim.keyframes(kf);
    EXPECT_EQ(anim.keyframes().size(), 3u);
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
    anim.keyframes({{0.0f, 0.0f, 0.0f, 0.0f, 1.0f}});

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
    anim.keyframes({
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
    anim.keyframes({
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
    anim.keyframes({
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
