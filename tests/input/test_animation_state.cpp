#include <fire_engine/input/animation_state.hpp>

#include <gtest/gtest.h>

using fire_engine::AnimationState;

// ==========================================================================
// Default Construction
// ==========================================================================

TEST(AnimationStateConstruction, DefaultHasNoActiveAnimation)
{
    AnimationState s;
    EXPECT_FALSE(s.hasActiveAnimation());
    EXPECT_FALSE(s.activeAnimation().has_value());
}

// ==========================================================================
// Accessors
// ==========================================================================

TEST(AnimationStateAccessors, SetActiveAnimation)
{
    AnimationState s;
    s.activeAnimation(2);
    EXPECT_TRUE(s.hasActiveAnimation());
    EXPECT_EQ(*s.activeAnimation(), 2);
}

TEST(AnimationStateAccessors, OverwriteActiveAnimation)
{
    AnimationState s;
    s.activeAnimation(1);
    s.activeAnimation(0);
    EXPECT_EQ(*s.activeAnimation(), 0);
}

// ==========================================================================
// Copy / Move
// ==========================================================================

TEST(AnimationStateCopy, CopyConstructCreatesIndependentCopy)
{
    AnimationState a;
    a.activeAnimation(3);

    AnimationState b{a};
    EXPECT_EQ(*b.activeAnimation(), 3);

    b.activeAnimation(5);
    EXPECT_EQ(*a.activeAnimation(), 3);
}

TEST(AnimationStateCopy, CopyAssign)
{
    AnimationState a;
    a.activeAnimation(7);
    AnimationState b;
    b = a;
    EXPECT_EQ(*b.activeAnimation(), 7);
}

TEST(AnimationStateMove, MoveConstructTransfersState)
{
    AnimationState a;
    a.activeAnimation(4);
    AnimationState b{std::move(a)};
    EXPECT_EQ(*b.activeAnimation(), 4);
}

// ==========================================================================
// Noexcept
// ==========================================================================

TEST(AnimationStateNoexcept, AllOperationsAreNoexcept)
{
    AnimationState s;

    static_assert(noexcept(AnimationState{}));
    static_assert(noexcept(s.activeAnimation()));
    static_assert(noexcept(s.hasActiveAnimation()));
    static_assert(noexcept(s.activeAnimation(0)));
    static_assert(std::is_nothrow_move_constructible_v<AnimationState>);
}
