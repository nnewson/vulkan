#include <fire_engine/input/controller_state.hpp>

#include <gtest/gtest.h>

using fire_engine::ControllerState;

// ==========================================================================
// Default Construction
// ==========================================================================

TEST(ControllerStateConstruction, DefaultDeltaPositionIsZero)
{
    ControllerState s;
    EXPECT_FLOAT_EQ(s.deltaPosition().x(), 0.0f);
    EXPECT_FLOAT_EQ(s.deltaPosition().y(), 0.0f);
    EXPECT_FLOAT_EQ(s.deltaPosition().z(), 0.0f);
}

TEST(ControllerStateConstruction, DefaultTimeIsZero)
{
    ControllerState s;
    EXPECT_DOUBLE_EQ(s.time(), 0.0);
}

// ==========================================================================
// Accessors
// ==========================================================================

TEST(ControllerStateAccessors, SetDeltaPosition)
{
    ControllerState s;
    s.deltaPosition({1.0f, 2.0f, 3.0f});
    EXPECT_FLOAT_EQ(s.deltaPosition().x(), 1.0f);
    EXPECT_FLOAT_EQ(s.deltaPosition().y(), 2.0f);
    EXPECT_FLOAT_EQ(s.deltaPosition().z(), 3.0f);
}

TEST(ControllerStateAccessors, SetTime)
{
    ControllerState s;
    s.time(1.5);
    EXPECT_DOUBLE_EQ(s.time(), 1.5);
}

// ==========================================================================
// Copy and Move Semantics
// ==========================================================================

TEST(ControllerStateCopy, CopyConstructCreatesIndependentCopy)
{
    ControllerState a;
    a.deltaPosition({1.0f, 2.0f, 3.0f});
    a.time(4.0);

    ControllerState b{a};
    EXPECT_FLOAT_EQ(b.deltaPosition().x(), 1.0f);
    EXPECT_DOUBLE_EQ(b.time(), 4.0);

    b.deltaPosition({9.0f, 0.0f, 0.0f});
    EXPECT_FLOAT_EQ(a.deltaPosition().x(), 1.0f);
}

TEST(ControllerStateCopy, CopyAssign)
{
    ControllerState a;
    a.deltaPosition({4.0f, 5.0f, 6.0f});
    ControllerState b;
    b = a;
    EXPECT_FLOAT_EQ(b.deltaPosition().x(), 4.0f);
}

TEST(ControllerStateMove, MoveConstructTransfersState)
{
    ControllerState a;
    a.deltaPosition({7.0f, 8.0f, 9.0f});
    a.time(2.0);

    ControllerState b{std::move(a)};
    EXPECT_FLOAT_EQ(b.deltaPosition().x(), 7.0f);
    EXPECT_DOUBLE_EQ(b.time(), 2.0);
}

// ==========================================================================
// Noexcept guarantees
// ==========================================================================

TEST(ControllerStateNoexcept, AllOperationsAreNoexcept)
{
    ControllerState s;

    static_assert(noexcept(ControllerState{}));
    static_assert(noexcept(s.deltaPosition()));
    static_assert(noexcept(s.deltaPosition({})));
    static_assert(noexcept(s.time()));
    static_assert(noexcept(s.time(0.0)));
    static_assert(std::is_nothrow_move_constructible_v<ControllerState>);
}
