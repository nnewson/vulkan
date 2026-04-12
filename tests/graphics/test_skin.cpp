#include <fire_engine/graphics/skin.hpp>
#include <fire_engine/input/camera_state.hpp>
#include <fire_engine/scene/node.hpp>

#include <gtest/gtest.h>

using fire_engine::CameraState;
using fire_engine::Mat4;
using fire_engine::Node;
using fire_engine::Skin;

// ==========================================================================
// Default Construction
// ==========================================================================

TEST(SkinConstruction, DefaultEmpty)
{
    Skin skin;
    EXPECT_TRUE(skin.empty());
    EXPECT_EQ(skin.jointCount(), 0u);
}

TEST(SkinConstruction, DefaultName)
{
    Skin skin;
    EXPECT_TRUE(skin.name().empty());
}

// ==========================================================================
// Name
// ==========================================================================

TEST(SkinName, SetAndGet)
{
    Skin skin;
    skin.name("TestSkin");
    EXPECT_EQ(skin.name(), "TestSkin");
}

// ==========================================================================
// Add Joints
// ==========================================================================

TEST(SkinAddJoint, SingleJoint)
{
    Skin skin;
    Node node("joint0");
    skin.addJoint(&node, Mat4::identity());
    EXPECT_EQ(skin.jointCount(), 1u);
    EXPECT_FALSE(skin.empty());
}

TEST(SkinAddJoint, MultipleJoints)
{
    Skin skin;
    Node n0("joint0");
    Node n1("joint1");
    Node n2("joint2");
    skin.addJoint(&n0, Mat4::identity());
    skin.addJoint(&n1, Mat4::identity());
    skin.addJoint(&n2, Mat4::identity());
    EXPECT_EQ(skin.jointCount(), 3u);
}

// ==========================================================================
// Compute Joint Matrices
// ==========================================================================

TEST(SkinComputeJointMatrices, EmptySkin)
{
    Skin skin;
    auto matrices = skin.computeJointMatrices();
    EXPECT_TRUE(matrices.empty());
}

TEST(SkinComputeJointMatrices, IdentityInverseBind)
{
    Skin skin;
    Node node("joint0");
    CameraState cs;
    node.update(cs, Mat4::identity());

    skin.addJoint(&node, Mat4::identity());
    auto matrices = skin.computeJointMatrices();

    ASSERT_EQ(matrices.size(), 1u);
    // world (identity) * inverseBind (identity) = identity
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            float expected = (row == col) ? 1.0f : 0.0f;
            float actual = matrices[0][row, col];
            EXPECT_FLOAT_EQ(actual, expected);
        }
    }
}

TEST(SkinComputeJointMatrices, WithTranslation)
{
    Skin skin;
    Node node("joint0");
    node.transform().position({5.0f, 0.0f, 0.0f});

    CameraState cs;
    node.update(cs, Mat4::identity());

    skin.addJoint(&node, Mat4::identity());
    auto matrices = skin.computeJointMatrices();

    ASSERT_EQ(matrices.size(), 1u);
    float tx = matrices[0][0, 3];
    EXPECT_FLOAT_EQ(tx, 5.0f);
}

// ==========================================================================
// Move Semantics
// ==========================================================================

TEST(SkinMove, MoveConstruction)
{
    Skin skin;
    Node n0("joint0");
    skin.addJoint(&n0, Mat4::identity());
    skin.name("TestSkin");

    Skin moved(std::move(skin));
    EXPECT_EQ(moved.jointCount(), 1u);
    EXPECT_EQ(moved.name(), "TestSkin");
}

TEST(SkinMove, MoveAssignment)
{
    Skin skin;
    Node n0("joint0");
    skin.addJoint(&n0, Mat4::identity());

    Skin other;
    other = std::move(skin);
    EXPECT_EQ(other.jointCount(), 1u);
}
