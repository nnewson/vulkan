#include <fire_engine/input/input_state.hpp>

#include <gtest/gtest.h>

using fire_engine::AnimationState;
using fire_engine::CameraState;
using fire_engine::InputState;
using fire_engine::Vec3;

// ==========================================================================
// Default Construction
// ==========================================================================

TEST(InputStateConstruction, DefaultCameraStateIsZero)
{
    InputState s;
    EXPECT_FLOAT_EQ(s.cameraState().deltaYaw(), 0.0f);
    EXPECT_FLOAT_EQ(s.cameraState().deltaPitch(), 0.0f);
    EXPECT_DOUBLE_EQ(s.time(), 0.0);
}

TEST(InputStateConstruction, DefaultAnimationStateHasNoActive)
{
    InputState s;
    EXPECT_FALSE(s.animationState().hasActiveAnimation());
}

// ==========================================================================
// Accessors
// ==========================================================================

TEST(InputStateAccessors, SetCameraState)
{
    InputState s;
    CameraState cs;
    cs.deltaYaw(1.5f);
    s.cameraState(cs);
    EXPECT_FLOAT_EQ(s.cameraState().deltaYaw(), 1.5f);
}

TEST(InputStateAccessors, SetAnimationState)
{
    InputState s;
    AnimationState as;
    as.activeAnimation(2);
    s.animationState(as);
    EXPECT_EQ(*s.animationState().activeAnimation(), 2);
}

TEST(InputStateAccessors, TimeConvenienceAccessor)
{
    InputState s;
    s.time(42.0);
    EXPECT_DOUBLE_EQ(s.time(), 42.0);
    EXPECT_DOUBLE_EQ(s.cameraState().time(), 42.0);
}

TEST(InputStateAccessors, MutableCameraStateRef)
{
    InputState s;
    s.cameraState().deltaPosition({1.0f, 2.0f, 3.0f});
    EXPECT_FLOAT_EQ(s.cameraState().deltaPosition().x(), 1.0f);
}

TEST(InputStateAccessors, MutableAnimationStateRef)
{
    InputState s;
    s.animationState().activeAnimation(1);
    EXPECT_EQ(*s.animationState().activeAnimation(), 1);
}

// ==========================================================================
// Copy / Move
// ==========================================================================

TEST(InputStateCopy, CopyConstructCreatesIndependentCopy)
{
    InputState a;
    a.time(10.0);
    a.animationState().activeAnimation(5);

    InputState b{a};
    EXPECT_DOUBLE_EQ(b.time(), 10.0);
    EXPECT_EQ(*b.animationState().activeAnimation(), 5);

    b.time(20.0);
    EXPECT_DOUBLE_EQ(a.time(), 10.0);
}

TEST(InputStateCopy, CopyAssign)
{
    InputState a;
    a.time(7.0);
    InputState b;
    b = a;
    EXPECT_DOUBLE_EQ(b.time(), 7.0);
}

TEST(InputStateMove, MoveConstructTransfersState)
{
    InputState a;
    a.time(33.0);
    a.animationState().activeAnimation(2);
    InputState b{std::move(a)};
    EXPECT_DOUBLE_EQ(b.time(), 33.0);
    EXPECT_EQ(*b.animationState().activeAnimation(), 2);
}

// ==========================================================================
// Noexcept
// ==========================================================================

TEST(InputStateNoexcept, AllOperationsAreNoexcept)
{
    InputState s;

    static_assert(noexcept(InputState{}));
    static_assert(noexcept(s.cameraState()));
    static_assert(noexcept(s.animationState()));
    static_assert(noexcept(s.time()));
    static_assert(noexcept(s.time(0.0)));
    static_assert(std::is_nothrow_move_constructible_v<InputState>);
}
