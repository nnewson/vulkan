#include <fire_engine/scene/camera.hpp>

#include <cmath>

#include <gtest/gtest.h>

using fire_engine::Camera;
using fire_engine::Vec3;

// ==========================================================================
// Construction
// ==========================================================================

TEST(CameraConstruction, DefaultPosition)
{
    Camera cam;
    EXPECT_FLOAT_EQ(cam.localPosition().x(), 2.0f);
    EXPECT_FLOAT_EQ(cam.localPosition().y(), 2.0f);
    EXPECT_FLOAT_EQ(cam.localPosition().z(), 2.0f);
}

TEST(CameraConstruction, DefaultYaw)
{
    Camera cam;
    EXPECT_FLOAT_EQ(cam.localYaw(), -2.356f);
}

TEST(CameraConstruction, DefaultPitch)
{
    Camera cam;
    EXPECT_FLOAT_EQ(cam.localPitch(), -0.615f);
}

// ==========================================================================
// Accessors
// ==========================================================================

TEST(CameraAccessors, SetPosition)
{
    Camera cam;
    cam.localPosition({5.0f, 10.0f, -3.0f});
    EXPECT_FLOAT_EQ(cam.localPosition().x(), 5.0f);
    EXPECT_FLOAT_EQ(cam.localPosition().y(), 10.0f);
    EXPECT_FLOAT_EQ(cam.localPosition().z(), -3.0f);
}

TEST(CameraAccessors, SetYaw)
{
    Camera cam;
    cam.localYaw(1.0f);
    EXPECT_FLOAT_EQ(cam.localYaw(), 1.0f);
}

TEST(CameraAccessors, SetPitch)
{
    Camera cam;
    cam.localPitch(0.5f);
    EXPECT_FLOAT_EQ(cam.localPitch(), 0.5f);
}

// ==========================================================================
// Pitch clamping
// ==========================================================================

TEST(CameraPitchClamp, ClampsAboveMax)
{
    Camera cam;
    cam.localPitch(2.0f);
    EXPECT_FLOAT_EQ(cam.localPitch(), 1.5f);
}

TEST(CameraPitchClamp, ClampsBelowMin)
{
    Camera cam;
    cam.localPitch(-2.0f);
    EXPECT_FLOAT_EQ(cam.localPitch(), -1.5f);
}

TEST(CameraPitchClamp, ExactMaxIsAllowed)
{
    Camera cam;
    cam.localPitch(1.5f);
    EXPECT_FLOAT_EQ(cam.localPitch(), 1.5f);
}

TEST(CameraPitchClamp, ExactMinIsAllowed)
{
    Camera cam;
    cam.localPitch(-1.5f);
    EXPECT_FLOAT_EQ(cam.localPitch(), -1.5f);
}

TEST(CameraPitchClamp, WithinRangeUnchanged)
{
    Camera cam;
    cam.localPitch(0.75f);
    EXPECT_FLOAT_EQ(cam.localPitch(), 0.75f);
}

// ==========================================================================
// Local target calculation
// ==========================================================================

TEST(CameraLocalTarget, ZeroPitchYawLooksAlongPositiveX)
{
    Camera cam;
    cam.localPosition({0.0f, 0.0f, 0.0f});
    cam.localYaw(0.0f);
    cam.localPitch(0.0f);

    Vec3 t = cam.localTarget();
    // cos(0)*cos(0) = 1, sin(0) = 0, cos(0)*sin(0) = 0
    EXPECT_NEAR(t.x(), 1.0f, 1e-6f);
    EXPECT_NEAR(t.y(), 0.0f, 1e-6f);
    EXPECT_NEAR(t.z(), 0.0f, 1e-6f);
}

TEST(CameraLocalTarget, YawRotatesInXZPlane)
{
    Camera cam;
    cam.localPosition({0.0f, 0.0f, 0.0f});
    cam.localPitch(0.0f);

    // yaw = pi/2 should look along +Z
    cam.localYaw(static_cast<float>(M_PI) / 2.0f);
    Vec3 t = cam.localTarget();
    EXPECT_NEAR(t.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(t.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(t.z(), 1.0f, 1e-5f);
}

TEST(CameraLocalTarget, PitchLooksUpward)
{
    Camera cam;
    cam.localPosition({0.0f, 0.0f, 0.0f});
    cam.localYaw(0.0f);

    // pitch = pi/4 should raise Y component
    cam.localPitch(static_cast<float>(M_PI) / 4.0f);
    Vec3 t = cam.localTarget();
    EXPECT_NEAR(t.y(), std::sin(static_cast<float>(M_PI) / 4.0f), 1e-6f);
    EXPECT_GT(t.y(), 0.0f);
}

TEST(CameraLocalTarget, TargetOffsetsFromPosition)
{
    Camera cam;
    cam.localPosition({10.0f, 20.0f, 30.0f});
    cam.localYaw(0.0f);
    cam.localPitch(0.0f);

    Vec3 t = cam.localTarget();
    // Target should be position + direction
    EXPECT_NEAR(t.x(), 11.0f, 1e-6f);
    EXPECT_NEAR(t.y(), 20.0f, 1e-6f);
    EXPECT_NEAR(t.z(), 30.0f, 1e-6f);
}

TEST(CameraLocalTarget, NegativeYawLooksAlongNegativeZ)
{
    Camera cam;
    cam.localPosition({0.0f, 0.0f, 0.0f});
    cam.localYaw(-static_cast<float>(M_PI) / 2.0f);
    cam.localPitch(0.0f);

    Vec3 t = cam.localTarget();
    EXPECT_NEAR(t.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(t.z(), -1.0f, 1e-5f);
}

// ==========================================================================
// Copy semantics
// ==========================================================================

TEST(CameraCopy, CopyConstructCreatesIndependentCopy)
{
    Camera a;
    a.localPosition({1.0f, 2.0f, 3.0f});
    a.localYaw(0.5f);
    a.localPitch(0.3f);

    Camera b{a};
    EXPECT_FLOAT_EQ(b.localPosition().x(), 1.0f);
    EXPECT_FLOAT_EQ(b.localYaw(), 0.5f);
    EXPECT_FLOAT_EQ(b.localPitch(), 0.3f);

    // Modifying b should not affect a
    b.localYaw(1.0f);
    EXPECT_FLOAT_EQ(a.localYaw(), 0.5f);
}

TEST(CameraCopy, CopyAssignCreatesIndependentCopy)
{
    Camera a;
    a.localPosition({4.0f, 5.0f, 6.0f});

    Camera b;
    b = a;
    EXPECT_FLOAT_EQ(b.localPosition().x(), 4.0f);

    b.localPosition({0.0f, 0.0f, 0.0f});
    EXPECT_FLOAT_EQ(a.localPosition().x(), 4.0f);
}

// ==========================================================================
// Move semantics
// ==========================================================================

TEST(CameraMove, MoveConstructTransfersState)
{
    Camera a;
    a.localPosition({7.0f, 8.0f, 9.0f});
    a.localYaw(1.2f);

    Camera b{std::move(a)};
    EXPECT_FLOAT_EQ(b.localPosition().x(), 7.0f);
    EXPECT_FLOAT_EQ(b.localYaw(), 1.2f);
}

TEST(CameraMove, MoveAssignTransfersState)
{
    Camera a;
    a.localPitch(0.8f);

    Camera b;
    b = std::move(a);
    EXPECT_FLOAT_EQ(b.localPitch(), 0.8f);
}
