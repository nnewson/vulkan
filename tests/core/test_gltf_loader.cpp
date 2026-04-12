#include <cmath>

#include <fastgltf/math.hpp>

#include <fire_engine/math/constants.hpp>
#include <fire_engine/scene/node.hpp>
#include <fire_engine/scene/transform.hpp>

#include <gtest/gtest.h>

using fire_engine::Mat4;
using fire_engine::Node;
using fire_engine::Vec3;

// ---------------------------------------------------------------------------
// Reproduce the quaternionToEuler logic used in GltfLoader (private static)
// so we can validate it independently.
// ---------------------------------------------------------------------------
static Vec3 quaternionToEuler(float qx, float qy, float qz, float qw)
{
    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    float roll = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (qw * qy - qz * qx);
    float pitch =
        (std::abs(sinp) >= 1.0f) ? std::copysign(3.14159265f / 2.0f, sinp) : std::asin(sinp);

    float siny_cosp = 2.0f * (qw * qz + qx * qy);
    float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
    float yaw = std::atan2(siny_cosp, cosy_cosp);

    return {pitch, yaw, roll};
}

// Applies a fastgltf matrix to a Node's Transform via decomposition,
// mirroring the logic in GltfLoader::applyTRS for the matrix branch.
static void applyMatrix(const fastgltf::math::fmat4x4& mat, Node& node)
{
    fastgltf::math::fvec3 scale;
    fastgltf::math::fquat rotation;
    fastgltf::math::fvec3 translation;
    fastgltf::math::decomposeTransformMatrix(mat, scale, rotation, translation);

    node.transform().position({translation.x(), translation.y(), translation.z()});
    node.transform().rotation(
        quaternionToEuler(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
    node.transform().scale({scale.x(), scale.y(), scale.z()});
}

// ==========================================================================
// quaternionToEuler
// ==========================================================================

TEST(QuaternionToEuler, IdentityQuaternionGivesZeroRotation)
{
    auto euler = quaternionToEuler(0, 0, 0, 1);
    EXPECT_NEAR(euler.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(euler.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(euler.z(), 0.0f, 1e-5f);
}

TEST(QuaternionToEuler, Rotate90DegreesAroundY)
{
    // Y-axis rotation at 90° hits gimbal lock (pitch = ±90°).
    // quaternionToEuler returns (pitch, yaw, roll) where pitch maps to the Y-axis
    // singularity. At exactly 90° the pitch saturates and yaw absorbs the remainder.
    float angle = fire_engine::pi / 2.0f;
    float qy = std::sin(angle / 2.0f);
    float qw = std::cos(angle / 2.0f);
    auto euler = quaternionToEuler(0, qy, 0, qw);
    // Pitch (euler.x) should be ~90° — this is the gimbal lock singularity
    EXPECT_NEAR(euler.x(), angle, 1e-3f);
}

TEST(QuaternionToEuler, Rotate90DegreesAroundX)
{
    float angle = fire_engine::pi / 2.0f;
    float qx = std::sin(angle / 2.0f);
    float qw = std::cos(angle / 2.0f);
    auto euler = quaternionToEuler(qx, 0, 0, qw);
    EXPECT_NEAR(euler.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(euler.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(euler.z(), angle, 1e-5f);
}

TEST(QuaternionToEuler, Rotate90DegreesAroundZ)
{
    float angle = fire_engine::pi / 2.0f;
    float qz = std::sin(angle / 2.0f);
    float qw = std::cos(angle / 2.0f);
    auto euler = quaternionToEuler(0, 0, qz, qw);
    EXPECT_NEAR(euler.x(), 0.0f, 1e-5f);
    EXPECT_NEAR(euler.y(), angle, 1e-5f);
    EXPECT_NEAR(euler.z(), 0.0f, 1e-5f);
}

// ==========================================================================
// Matrix decomposition via applyMatrix (mirrors GltfLoader::applyTRS)
// ==========================================================================

TEST(MatrixDecomposition, IdentityMatrixGivesDefaultTransform)
{
    fastgltf::math::fmat4x4 mat{};
    mat.col(0) = {1, 0, 0, 0};
    mat.col(1) = {0, 1, 0, 0};
    mat.col(2) = {0, 0, 1, 0};
    mat.col(3) = {0, 0, 0, 1};

    Node node("test");
    applyMatrix(mat, node);

    EXPECT_NEAR(node.transform().position().x(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().y(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().z(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().x(), 1.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().y(), 1.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().z(), 1.0f, 1e-5f);
}

TEST(MatrixDecomposition, PureTranslation)
{
    fastgltf::math::fmat4x4 mat{};
    mat.col(0) = {1, 0, 0, 0};
    mat.col(1) = {0, 1, 0, 0};
    mat.col(2) = {0, 0, 1, 0};
    mat.col(3) = {5, 10, 15, 1};

    Node node("test");
    applyMatrix(mat, node);

    EXPECT_NEAR(node.transform().position().x(), 5.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().y(), 10.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().z(), 15.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().x(), 1.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().y(), 1.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().z(), 1.0f, 1e-5f);
}

TEST(MatrixDecomposition, PureScale)
{
    fastgltf::math::fmat4x4 mat{};
    mat.col(0) = {2, 0, 0, 0};
    mat.col(1) = {0, 3, 0, 0};
    mat.col(2) = {0, 0, 4, 0};
    mat.col(3) = {0, 0, 0, 1};

    Node node("test");
    applyMatrix(mat, node);

    EXPECT_NEAR(node.transform().position().x(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().y(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().z(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().x(), 2.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().y(), 3.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().z(), 4.0f, 1e-5f);
}

TEST(MatrixDecomposition, TranslationAndScale)
{
    fastgltf::math::fmat4x4 mat{};
    mat.col(0) = {2, 0, 0, 0};
    mat.col(1) = {0, 3, 0, 0};
    mat.col(2) = {0, 0, 4, 0};
    mat.col(3) = {1, 2, 3, 1};

    Node node("test");
    applyMatrix(mat, node);

    EXPECT_NEAR(node.transform().position().x(), 1.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().y(), 2.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().z(), 3.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().x(), 2.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().y(), 3.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().z(), 4.0f, 1e-5f);
}

TEST(MatrixDecomposition, ZUpRotationMatrix)
{
    // From RiggedSimple.gltf Node 0 "Z_UP":
    // [1,0,0,0, 0,0,-1,0, 0,1,0,0, 0,0,0,1] (column-major)
    fastgltf::math::fmat4x4 mat{};
    mat.col(0) = {1, 0, 0, 0};
    mat.col(1) = {0, 0, -1, 0};
    mat.col(2) = {0, 1, 0, 0};
    mat.col(3) = {0, 0, 0, 1};

    Node node("Z_UP");
    applyMatrix(mat, node);

    EXPECT_NEAR(node.transform().position().x(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().y(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().z(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().x(), 1.0f, 1e-4f);
    EXPECT_NEAR(node.transform().scale().y(), 1.0f, 1e-4f);
    EXPECT_NEAR(node.transform().scale().z(), 1.0f, 1e-4f);

    // Rotation should be non-zero (this is a 90° rotation around X)
    auto rot = node.transform().rotation();
    EXPECT_TRUE(std::abs(rot.x()) > 0.01f || std::abs(rot.y()) > 0.01f ||
                std::abs(rot.z()) > 0.01f);
}

TEST(MatrixDecomposition, ArmatureRotationMatrix)
{
    // From RiggedSimple.gltf Node 1 "Armature":
    // [-4.37e-08,-1,0,0, 1,-4.37e-08,0,0, 0,0,1,0, 0,0,0,1]
    float eps = -4.371139894487897e-08f;
    fastgltf::math::fmat4x4 mat{};
    mat.col(0) = {eps, 1, 0, 0};
    mat.col(1) = {-1, eps, 0, 0};
    mat.col(2) = {0, 0, 1, 0};
    mat.col(3) = {0, 0, 0, 1};

    Node node("Armature");
    applyMatrix(mat, node);

    EXPECT_NEAR(node.transform().position().x(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().y(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().z(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().x(), 1.0f, 1e-4f);
    EXPECT_NEAR(node.transform().scale().y(), 1.0f, 1e-4f);
    EXPECT_NEAR(node.transform().scale().z(), 1.0f, 1e-4f);
}

TEST(MatrixDecomposition, BoneTranslationMatrix)
{
    // From RiggedSimple.gltf Node 3 "Bone":
    // [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,-1.36e-07,-4.18,1]
    fastgltf::math::fmat4x4 mat{};
    mat.col(0) = {1, 0, 0, 0};
    mat.col(1) = {0, 1, 0, 0};
    mat.col(2) = {0, 0, 1, 0};
    mat.col(3) = {0, -1.3597299641787688e-07f, -4.1803297996521f, 1};

    Node node("Bone");
    applyMatrix(mat, node);

    EXPECT_NEAR(node.transform().position().x(), 0.0f, 1e-5f);
    EXPECT_NEAR(node.transform().position().y(), -1.3597299641787688e-07f, 1e-10f);
    EXPECT_NEAR(node.transform().position().z(), -4.1803297996521f, 1e-3f);
    EXPECT_NEAR(node.transform().scale().x(), 1.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().y(), 1.0f, 1e-5f);
    EXPECT_NEAR(node.transform().scale().z(), 1.0f, 1e-5f);
}

// ==========================================================================
// Verify Transform::update produces correct world matrix after decomposition
// ==========================================================================

TEST(MatrixDecomposition, UpdateProducesCorrectWorldTranslation)
{
    fastgltf::math::fmat4x4 mat{};
    mat.col(0) = {1, 0, 0, 0};
    mat.col(1) = {0, 1, 0, 0};
    mat.col(2) = {0, 0, 1, 0};
    mat.col(3) = {3, 7, 11, 1};

    Node node("test");
    applyMatrix(mat, node);
    node.transform().update(Mat4::identity());

    EXPECT_NEAR((node.transform().world()[0, 3]), 3.0f, 1e-5f);
    EXPECT_NEAR((node.transform().world()[1, 3]), 7.0f, 1e-5f);
    EXPECT_NEAR((node.transform().world()[2, 3]), 11.0f, 1e-5f);
}
