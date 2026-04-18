#include <fire_engine/input/camera_state.hpp>

#include <gtest/gtest.h>

using fire_engine::CameraState;

// ==========================================================================
// Default Construction
// ==========================================================================

TEST(CameraStateConstruction, DefaultDeltaPositionIsZero)
{
    CameraState s;
    EXPECT_FLOAT_EQ(s.deltaPosition().x(), 0.0f);
    EXPECT_FLOAT_EQ(s.deltaPosition().y(), 0.0f);
    EXPECT_FLOAT_EQ(s.deltaPosition().z(), 0.0f);
}

TEST(CameraStateConstruction, DefaultDeltaYawIsZero)
{
    CameraState s;
    EXPECT_FLOAT_EQ(s.deltaYaw(), 0.0f);
}

TEST(CameraStateConstruction, DefaultDeltaPitchIsZero)
{
    CameraState s;
    EXPECT_FLOAT_EQ(s.deltaPitch(), 0.0f);
}

TEST(CameraStateConstruction, DefaultTimeIsZero)
{
    CameraState s;
    EXPECT_DOUBLE_EQ(s.time(), 0.0);
}

TEST(CameraStateConstruction, DefaultDeltaZoomIsZero)
{
    CameraState s;
    EXPECT_FLOAT_EQ(s.deltaZoom(), 0.0f);
}

// ==========================================================================
// Accessors
// ==========================================================================

TEST(CameraStateAccessors, SetDeltaPosition)
{
    CameraState s;
    s.deltaPosition({1.0f, 2.0f, 3.0f});
    EXPECT_FLOAT_EQ(s.deltaPosition().x(), 1.0f);
    EXPECT_FLOAT_EQ(s.deltaPosition().y(), 2.0f);
    EXPECT_FLOAT_EQ(s.deltaPosition().z(), 3.0f);
}

TEST(CameraStateAccessors, SetDeltaYaw)
{
    CameraState s;
    s.deltaYaw(0.5f);
    EXPECT_FLOAT_EQ(s.deltaYaw(), 0.5f);
}

TEST(CameraStateAccessors, SetDeltaPitch)
{
    CameraState s;
    s.deltaPitch(-0.3f);
    EXPECT_FLOAT_EQ(s.deltaPitch(), -0.3f);
}

TEST(CameraStateAccessors, SetTime)
{
    CameraState s;
    s.time(1.5);
    EXPECT_DOUBLE_EQ(s.time(), 1.5);
}

TEST(CameraStateAccessors, OverwritePreviousValues)
{
    CameraState s;
    s.deltaYaw(1.0f);
    s.deltaYaw(2.0f);
    EXPECT_FLOAT_EQ(s.deltaYaw(), 2.0f);
}

TEST(CameraStateAccessors, SetDeltaZoom)
{
    CameraState s;
    s.deltaZoom(2.5f);
    EXPECT_FLOAT_EQ(s.deltaZoom(), 2.5f);
}

// ==========================================================================
// Copy and Move Semantics
// ==========================================================================

TEST(CameraStateCopy, CopyConstructCreatesIndependentCopy)
{
    CameraState a;
    a.deltaPosition({1.0f, 2.0f, 3.0f});
    a.deltaYaw(0.5f);
    a.deltaPitch(-0.3f);

    CameraState b{a};
    EXPECT_FLOAT_EQ(b.deltaPosition().x(), 1.0f);
    EXPECT_FLOAT_EQ(b.deltaYaw(), 0.5f);
    EXPECT_FLOAT_EQ(b.deltaPitch(), -0.3f);

    b.deltaYaw(9.0f);
    EXPECT_FLOAT_EQ(a.deltaYaw(), 0.5f);
}

TEST(CameraStateCopy, CopyAssign)
{
    CameraState a;
    a.deltaPosition({4.0f, 5.0f, 6.0f});
    CameraState b;
    b = a;
    EXPECT_FLOAT_EQ(b.deltaPosition().x(), 4.0f);
}

TEST(CameraStateMove, MoveConstructTransfersState)
{
    CameraState a;
    a.deltaPosition({7.0f, 8.0f, 9.0f});
    a.deltaYaw(1.5f);

    CameraState b{std::move(a)};
    EXPECT_FLOAT_EQ(b.deltaPosition().x(), 7.0f);
    EXPECT_FLOAT_EQ(b.deltaYaw(), 1.5f);
}

// ==========================================================================
// Noexcept guarantees
// ==========================================================================

TEST(CameraStateNoexcept, AllOperationsAreNoexcept)
{
    CameraState s;

    static_assert(noexcept(CameraState{}));
    static_assert(noexcept(s.deltaPosition()));
    static_assert(noexcept(s.deltaYaw()));
    static_assert(noexcept(s.deltaPitch()));
    static_assert(noexcept(s.deltaPosition({})));
    static_assert(noexcept(s.deltaYaw(0.0f)));
    static_assert(noexcept(s.deltaPitch(0.0f)));
    static_assert(noexcept(s.time()));
    static_assert(noexcept(s.time(0.0)));
    static_assert(noexcept(s.deltaZoom()));
    static_assert(noexcept(s.deltaZoom(0.0f)));
}
