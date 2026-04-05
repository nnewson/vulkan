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
    EXPECT_FLOAT_EQ(cam.position().x(), 2.0f);
    EXPECT_FLOAT_EQ(cam.position().y(), 2.0f);
    EXPECT_FLOAT_EQ(cam.position().z(), 2.0f);
}

TEST(CameraConstruction, DefaultYaw)
{
    Camera cam;
    EXPECT_FLOAT_EQ(cam.yaw(), -2.356f);
}

TEST(CameraConstruction, DefaultPitch)
{
    Camera cam;
    EXPECT_FLOAT_EQ(cam.pitch(), -0.615f);
}

// ==========================================================================
// Accessors
// ==========================================================================

TEST(CameraAccessors, SetPosition)
{
    Camera cam;
    cam.position({5.0f, 10.0f, -3.0f});
    EXPECT_FLOAT_EQ(cam.position().x(), 5.0f);
    EXPECT_FLOAT_EQ(cam.position().y(), 10.0f);
    EXPECT_FLOAT_EQ(cam.position().z(), -3.0f);
}

TEST(CameraAccessors, SetYaw)
{
    Camera cam;
    cam.yaw(1.0f);
    EXPECT_FLOAT_EQ(cam.yaw(), 1.0f);
}

TEST(CameraAccessors, SetPitch)
{
    Camera cam;
    cam.pitch(0.5f);
    EXPECT_FLOAT_EQ(cam.pitch(), 0.5f);
}

// ==========================================================================
// Pitch clamping
// ==========================================================================

TEST(CameraPitchClamp, ClampsAboveMax)
{
    Camera cam;
    cam.pitch(2.0f);
    EXPECT_FLOAT_EQ(cam.pitch(), 1.5f);
}

TEST(CameraPitchClamp, ClampsBelowMin)
{
    Camera cam;
    cam.pitch(-2.0f);
    EXPECT_FLOAT_EQ(cam.pitch(), -1.5f);
}

TEST(CameraPitchClamp, ExactMaxIsAllowed)
{
    Camera cam;
    cam.pitch(1.5f);
    EXPECT_FLOAT_EQ(cam.pitch(), 1.5f);
}

TEST(CameraPitchClamp, ExactMinIsAllowed)
{
    Camera cam;
    cam.pitch(-1.5f);
    EXPECT_FLOAT_EQ(cam.pitch(), -1.5f);
}

TEST(CameraPitchClamp, WithinRangeUnchanged)
{
    Camera cam;
    cam.pitch(0.75f);
    EXPECT_FLOAT_EQ(cam.pitch(), 0.75f);
}

// ==========================================================================
// Target calculation
// ==========================================================================

TEST(CameraTarget, ZeroPitchYawLooksAlongPositiveX)
{
    Camera cam;
    cam.position({0.0f, 0.0f, 0.0f});
    cam.yaw(0.0f);
    cam.pitch(0.0f);

    Vec3 t = cam.target();
    // cos(0)*cos(0) = 1, sin(0) = 0, cos(0)*sin(0) = 0
    EXPECT_NEAR(t.x(), 1.0f, 1e-6f);
    EXPECT_NEAR(t.y(), 0.0f, 1e-6f);
    EXPECT_NEAR(t.z(), 0.0f, 1e-6f);
}

TEST(CameraTarget, YawRotatesInXZPlane)
{
    Camera cam;
    cam.position({0.0f, 0.0f, 0.0f});
    cam.pitch(0.0f);

    // yaw = pi/2 should look along +Z
    cam.yaw(static_cast<float>(M_PI) / 2.0f);
    Vec3 t = cam.target();
    EXPECT_NEAR(t.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(t.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(t.z(), 1.0f, 1e-5f);
}

TEST(CameraTarget, PitchLooksUpward)
{
    Camera cam;
    cam.position({0.0f, 0.0f, 0.0f});
    cam.yaw(0.0f);

    // pitch = pi/4 should raise Y component
    cam.pitch(static_cast<float>(M_PI) / 4.0f);
    Vec3 t = cam.target();
    EXPECT_NEAR(t.y(), std::sin(static_cast<float>(M_PI) / 4.0f), 1e-6f);
    EXPECT_GT(t.y(), 0.0f);
}

TEST(CameraTarget, TargetOffsetsFromPosition)
{
    Camera cam;
    cam.position({10.0f, 20.0f, 30.0f});
    cam.yaw(0.0f);
    cam.pitch(0.0f);

    Vec3 t = cam.target();
    // Target should be position + direction
    EXPECT_NEAR(t.x(), 11.0f, 1e-6f);
    EXPECT_NEAR(t.y(), 20.0f, 1e-6f);
    EXPECT_NEAR(t.z(), 30.0f, 1e-6f);
}

TEST(CameraTarget, NegativeYawLooksAlongNegativeZ)
{
    Camera cam;
    cam.position({0.0f, 0.0f, 0.0f});
    cam.yaw(-static_cast<float>(M_PI) / 2.0f);
    cam.pitch(0.0f);

    Vec3 t = cam.target();
    EXPECT_NEAR(t.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(t.z(), -1.0f, 1e-5f);
}

// ==========================================================================
// Copy semantics
// ==========================================================================

TEST(CameraCopy, CopyConstructCreatesIndependentCopy)
{
    Camera a;
    a.position({1.0f, 2.0f, 3.0f});
    a.yaw(0.5f);
    a.pitch(0.3f);

    Camera b{a};
    EXPECT_FLOAT_EQ(b.position().x(), 1.0f);
    EXPECT_FLOAT_EQ(b.yaw(), 0.5f);
    EXPECT_FLOAT_EQ(b.pitch(), 0.3f);

    // Modifying b should not affect a
    b.yaw(1.0f);
    EXPECT_FLOAT_EQ(a.yaw(), 0.5f);
}

TEST(CameraCopy, CopyAssignCreatesIndependentCopy)
{
    Camera a;
    a.position({4.0f, 5.0f, 6.0f});

    Camera b;
    b = a;
    EXPECT_FLOAT_EQ(b.position().x(), 4.0f);

    b.position({0.0f, 0.0f, 0.0f});
    EXPECT_FLOAT_EQ(a.position().x(), 4.0f);
}

// ==========================================================================
// Move semantics
// ==========================================================================

TEST(CameraMove, MoveConstructTransfersState)
{
    Camera a;
    a.position({7.0f, 8.0f, 9.0f});
    a.yaw(1.2f);

    Camera b{std::move(a)};
    EXPECT_FLOAT_EQ(b.position().x(), 7.0f);
    EXPECT_FLOAT_EQ(b.yaw(), 1.2f);
}

TEST(CameraMove, MoveAssignTransfersState)
{
    Camera a;
    a.pitch(0.8f);

    Camera b;
    b = std::move(a);
    EXPECT_FLOAT_EQ(b.pitch(), 0.8f);
}
