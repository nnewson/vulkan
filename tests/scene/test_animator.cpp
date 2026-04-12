#include <fire_engine/scene/animator.hpp>

#include <cmath>

#include <fire_engine/animation/linear_animation.hpp>

#include <gtest/gtest.h>

using fire_engine::Animator;
using fire_engine::CameraState;
using fire_engine::LinearAnimation;
using fire_engine::Mat4;
using fire_engine::Transform;

// ==========================================================================
// Default Construction
// ==========================================================================

TEST(Animator, DefaultConstructionHasNullAnimation)
{
    Animator a;
    EXPECT_EQ(a.animation(), nullptr);
}

// ==========================================================================
// Animation Sampling via Update
// ==========================================================================

TEST(Animator, UpdateWithNullAnimationProducesIdentity)
{
    Animator a;
    CameraState state;
    Transform transform;
    state.time(1.0);
    a.update(state, transform);

    int dummy = 0;
    auto& ctx = reinterpret_cast<fire_engine::RenderContext&>(dummy);
    Mat4 world = Mat4::identity();
    Mat4 result = a.render(ctx, world);

    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            EXPECT_NEAR((result[r, c]), (world[r, c]), 1e-5f);
        }
    }
}

TEST(Animator, UpdateWithNoKeyframesProducesIdentity)
{
    Animator a;
    LinearAnimation anim;
    a.animation(&anim);

    CameraState state;
    Transform transform;
    state.time(1.0);
    a.update(state, transform);

    int dummy = 0;
    auto& ctx = reinterpret_cast<fire_engine::RenderContext&>(dummy);
    Mat4 world = Mat4::identity();
    Mat4 result = a.render(ctx, world);

    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            EXPECT_NEAR((result[r, c]), (world[r, c]), 1e-5f);
        }
    }
}

TEST(Animator, UpdateSamplesAnimationAtElapsedTime)
{
    Animator a;
    LinearAnimation anim;

    // 90 degrees around Y over 2 seconds
    float s45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    float c45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    anim.rotationKeyframes({
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {2.0f, 0.0f, s45, 0.0f, c45},
    });
    a.animation(&anim);

    CameraState state;
    Transform transform;

    // First update at t=10.0 initialises startTime
    state.time(10.0);
    a.update(state, transform);

    // Second update at t=11.0 -> elapsed = 1.0s -> midpoint of animation (45 deg Y)
    state.time(11.0);
    a.update(state, transform);

    int dummy = 0;
    auto& ctx = reinterpret_cast<fire_engine::RenderContext&>(dummy);
    Mat4 result = a.render(ctx, Mat4::identity());

    float cos45 = std::cos(static_cast<float>(M_PI) / 4.0f);
    float sin45 = std::sin(static_cast<float>(M_PI) / 4.0f);
    EXPECT_NEAR((result[0, 0]), cos45, 1e-4f);
    EXPECT_NEAR((result[0, 2]), sin45, 1e-4f);
    EXPECT_NEAR((result[2, 0]), -sin45, 1e-4f);
    EXPECT_NEAR((result[2, 2]), cos45, 1e-4f);
}

TEST(Animator, RenderAppliesWorldMatrix)
{
    Animator a;
    LinearAnimation anim;
    a.animation(&anim);

    CameraState state;
    Transform transform;
    state.time(0.0);
    a.update(state, transform);

    // Scale world by 2
    Mat4 world;
    world[0, 0] = 2.0f;
    world[1, 1] = 2.0f;
    world[2, 2] = 2.0f;
    world[3, 3] = 1.0f;

    fire_engine::RenderContext* ctx = nullptr;
    Mat4 result = a.render(*ctx, world);

    // With identity animation, result should equal world
    EXPECT_NEAR((result[0, 0]), 2.0f, 1e-5f);
    EXPECT_NEAR((result[1, 1]), 2.0f, 1e-5f);
    EXPECT_NEAR((result[2, 2]), 2.0f, 1e-5f);
}
