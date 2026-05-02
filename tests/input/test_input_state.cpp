#include <fire_engine/input/input_state.hpp>

#include <gtest/gtest.h>

using fire_engine::AnimationState;
using fire_engine::CameraState;
using fire_engine::ControllerState;
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

TEST(InputStateConstruction, DefaultControllerStateIsZero)
{
    InputState s;
    EXPECT_FLOAT_EQ(s.controllerState().deltaPosition().x(), 0.0f);
    EXPECT_DOUBLE_EQ(s.controllerState().time(), 0.0);
}

TEST(InputStateConstruction, DefaultDeltaTimeIsZero)
{
    InputState s;
    EXPECT_FLOAT_EQ(s.deltaTime(), 0.0f);
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

TEST(InputStateAccessors, SetControllerState)
{
    InputState s;
    ControllerState cs;
    cs.deltaPosition({1.0f, 0.0f, 0.0f});
    s.controllerState(cs);
    EXPECT_FLOAT_EQ(s.controllerState().deltaPosition().x(), 1.0f);
}

TEST(InputStateAccessors, TimeConvenienceAccessor)
{
    InputState s;
    s.time(42.0);
    EXPECT_DOUBLE_EQ(s.time(), 42.0);
    EXPECT_DOUBLE_EQ(s.cameraState().time(), 42.0);
    EXPECT_DOUBLE_EQ(s.controllerState().time(), 42.0);
}

TEST(InputStateAccessors, SetDeltaTime)
{
    InputState s;
    s.deltaTime(1.25f);
    EXPECT_FLOAT_EQ(s.deltaTime(), 1.25f);
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

TEST(InputStateAccessors, MutableControllerStateRef)
{
    InputState s;
    s.controllerState().deltaPosition({2.0f, 0.0f, 0.0f});
    EXPECT_FLOAT_EQ(s.controllerState().deltaPosition().x(), 2.0f);
}

// ==========================================================================
// Copy / Move
// ==========================================================================

TEST(InputStateCopy, CopyConstructCreatesIndependentCopy)
{
    InputState a;
    a.time(10.0);
    a.animationState().activeAnimation(5);
    a.controllerState().deltaPosition({1.0f, 0.0f, 0.0f});

    InputState b{a};
    EXPECT_DOUBLE_EQ(b.time(), 10.0);
    EXPECT_EQ(*b.animationState().activeAnimation(), 5);
    EXPECT_FLOAT_EQ(b.controllerState().deltaPosition().x(), 1.0f);

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
    a.controllerState().deltaPosition({3.0f, 0.0f, 0.0f});
    InputState b{std::move(a)};
    EXPECT_DOUBLE_EQ(b.time(), 33.0);
    EXPECT_EQ(*b.animationState().activeAnimation(), 2);
    EXPECT_FLOAT_EQ(b.controllerState().deltaPosition().x(), 3.0f);
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
    static_assert(noexcept(s.controllerState()));
    static_assert(noexcept(s.deltaTime()));
    static_assert(noexcept(s.deltaTime(0.0f)));
    static_assert(noexcept(s.time()));
    static_assert(noexcept(s.time(0.0)));
    static_assert(std::is_nothrow_move_constructible_v<InputState>);
}
