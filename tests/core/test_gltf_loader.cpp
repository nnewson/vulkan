#include <cmath>

#include <fastgltf/math.hpp>
#include <fastgltf/types.hpp>

#include <fire_engine/math/constants.hpp>
#include <fire_engine/scene/node.hpp>
#include <fire_engine/scene/transform.hpp>

#include <gtest/gtest.h>

using fire_engine::Mat4;
using fire_engine::Node;

// Applies a fastgltf matrix to a Node's Transform via decomposition,
// mirroring the logic in GltfLoader::applyTRS for the matrix branch.
static void applyMatrix(const fastgltf::math::fmat4x4& mat, Node& node)
{
    fastgltf::math::fvec3 scale;
    fastgltf::math::fquat rotation;
    fastgltf::math::fvec3 translation;
    fastgltf::math::decomposeTransformMatrix(mat, scale, rotation, translation);

    node.transform().position({translation.x(), translation.y(), translation.z()});
    node.transform().rotation({rotation.x(), rotation.y(), rotation.z(), rotation.w()});
    node.transform().scale({scale.x(), scale.y(), scale.z()});
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

    // Rotation should be non-identity (this is a 90° rotation around X)
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

// ==========================================================================
// TRS rotation round-trip — the loader must store the glTF quaternion
// verbatim on the Node's Transform, so the rendered rotation matches the
// source asset without passing through an intermediate Euler representation.
// ==========================================================================

TEST(TRSRotation, DecalBlendQuaternionRoundTrip)
{
    // AlphaBlendModeTest DecalBlend/DecalOpaque: pure X-axis rotation of ~-56°.
    // Applying the quaternion directly must land the rotation on the X axis,
    // not permute it onto Z as the old Euler path did.
    fastgltf::Node gltf;
    fastgltf::TRS trsInit{};
    trsInit.translation = fastgltf::math::fvec3{0.0f, 0.0f, 0.4090209901332855f};
    trsInit.rotation = fastgltf::math::fquat{-0.47185850143432617f, 0.0f, 0.0f,
                                              0.88167440891265869f};
    trsInit.scale = fastgltf::math::fvec3{1.0f, 1.0f, 1.0f};
    gltf.transform = trsInit;

    Node node("DecalBlend");
    // Mirror the TRS branch of GltfLoader::applyTRS inline so this test
    // exercises the stored-quaternion contract without needing access to the
    // private static.
    auto* trs = std::get_if<fastgltf::TRS>(&gltf.transform);
    ASSERT_NE(trs, nullptr);
    node.transform().position(
        {trs->translation.x(), trs->translation.y(), trs->translation.z()});
    node.transform().rotation(
        {trs->rotation.x(), trs->rotation.y(), trs->rotation.z(), trs->rotation.w()});
    node.transform().scale({trs->scale.x(), trs->scale.y(), trs->scale.z()});

    auto rot = node.transform().rotation();
    EXPECT_FLOAT_EQ(rot.x(), -0.47185850143432617f);
    EXPECT_FLOAT_EQ(rot.y(), 0.0f);
    EXPECT_FLOAT_EQ(rot.z(), 0.0f);
    EXPECT_FLOAT_EQ(rot.w(), 0.88167440891265869f);

    // Extrinsic-XYZ Euler extraction must place the rotation on the X axis.
    auto e = rot.toEulerXYZ();
    EXPECT_NEAR(e.x(), -0.98279f, 1e-4f);
    EXPECT_NEAR(e.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(e.z(), 0.0f, 1e-5f);
}
