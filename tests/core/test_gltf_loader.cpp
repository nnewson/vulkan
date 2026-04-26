#include <cmath>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <fastgltf/core.hpp>
#include <fastgltf/math.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <fire_engine/core/gltf_loader.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/math/constants.hpp>
#include <fire_engine/math/vec3.hpp>
#include <fire_engine/scene/node.hpp>
#include <fire_engine/scene/transform.hpp>

#include <gtest/gtest.h>

using fire_engine::AlphaMode;
using fire_engine::GltfLoader;
using fire_engine::Mat4;
using fire_engine::Material;
using fire_engine::Node;
using fire_engine::Vec3;

// Mirrors the occlusion-strength translation performed inside
// GltfLoader::loadMaterial so it can be exercised without a GPU.
static void applyOcclusionStrength(const fastgltf::Material& gltfMat, Material& material)
{
    if (gltfMat.occlusionTexture.has_value())
    {
        material.occlusionStrength(static_cast<float>(gltfMat.occlusionTexture.value().strength));
    }
}

// Mirrors KHR_materials_emissive_strength translation: emissiveFactor is
// scaled by emissiveStrength at load time. Default emissiveStrength = 1.0.
static void applyEmissiveStrength(const fastgltf::Material& gltfMat, Material& material)
{
    const float strength = static_cast<float>(gltfMat.emissiveStrength);
    material.emissive({static_cast<float>(gltfMat.emissiveFactor.x()) * strength,
                       static_cast<float>(gltfMat.emissiveFactor.y()) * strength,
                       static_cast<float>(gltfMat.emissiveFactor.z()) * strength});
}

// Mirrors KHR_texture_transform translation: when a TextureInfo carries the
// extension, copy offset/scale/rotation onto the matching Material slot.
static fire_engine::UvTransform readUvTransform(const fastgltf::TextureInfo& info)
{
    fire_engine::UvTransform t;
    if (info.transform)
    {
        const auto& src = *info.transform;
        t.offsetX = static_cast<float>(src.uvOffset.x());
        t.offsetY = static_cast<float>(src.uvOffset.y());
        t.scaleX = static_cast<float>(src.uvScale.x());
        t.scaleY = static_cast<float>(src.uvScale.y());
        t.rotation = static_cast<float>(src.rotation);
    }
    return t;
}

// Mirrors per-slot texCoord-index translation: each glTF TextureInfo names a
// texCoord index (0 = TEXCOORD_0 default, 1 = TEXCOORD_1).
static void applyTexCoordIndices(const fastgltf::Material& gltfMat, Material& material)
{
    if (gltfMat.pbrData.baseColorTexture.has_value())
    {
        material.baseColorTexCoord(
            static_cast<int>(gltfMat.pbrData.baseColorTexture.value().texCoordIndex));
    }
    if (gltfMat.emissiveTexture.has_value())
    {
        material.emissiveTexCoord(
            static_cast<int>(gltfMat.emissiveTexture.value().texCoordIndex));
    }
    if (gltfMat.normalTexture.has_value())
    {
        material.normalTexCoord(
            static_cast<int>(gltfMat.normalTexture.value().texCoordIndex));
    }
    if (gltfMat.pbrData.metallicRoughnessTexture.has_value())
    {
        material.metallicRoughnessTexCoord(
            static_cast<int>(gltfMat.pbrData.metallicRoughnessTexture.value().texCoordIndex));
    }
    if (gltfMat.occlusionTexture.has_value())
    {
        material.occlusionTexCoord(
            static_cast<int>(gltfMat.occlusionTexture.value().texCoordIndex));
    }
}

// Mirrors the alpha-mode translation performed inside
// GltfLoader::loadMaterial so the translation can be exercised without
// needing a GPU-backed Resources object.
static void applyAlphaFields(const fastgltf::Material& gltfMat, Material& material)
{
    switch (gltfMat.alphaMode)
    {
    case fastgltf::AlphaMode::Opaque:
        material.alphaMode(AlphaMode::Opaque);
        break;
    case fastgltf::AlphaMode::Mask:
        material.alphaMode(AlphaMode::Mask);
        break;
    case fastgltf::AlphaMode::Blend:
        material.alphaMode(AlphaMode::Blend);
        break;
    }
    material.alphaCutoff(static_cast<float>(gltfMat.alphaCutoff));
    material.doubleSided(gltfMat.doubleSided);
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
    trsInit.rotation =
        fastgltf::math::fquat{-0.47185850143432617f, 0.0f, 0.0f, 0.88167440891265869f};
    trsInit.scale = fastgltf::math::fvec3{1.0f, 1.0f, 1.0f};
    gltf.transform = trsInit;

    Node node("DecalBlend");
    // Mirror the TRS branch of GltfLoader::applyTRS inline so this test
    // exercises the stored-quaternion contract without needing access to the
    // private static.
    auto* trs = std::get_if<fastgltf::TRS>(&gltf.transform);
    ASSERT_NE(trs, nullptr);
    node.transform().position({trs->translation.x(), trs->translation.y(), trs->translation.z()});
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

TEST(GltfFixture, MinimalTriangleFixtureParses)
{
    auto gltfPath = std::filesystem::path("test_assets/minimal_triangle.gltf");

    fastgltf::Parser parser;
    auto dataResult = fastgltf::GltfDataBuffer::FromPath(gltfPath);
    ASSERT_EQ(dataResult.error(), fastgltf::Error::None);

    auto result =
        parser.loadGltf(dataResult.get(), gltfPath.parent_path(), fastgltf::Options::None);
    ASSERT_EQ(result.error(), fastgltf::Error::None);

    const auto& asset = result.get();
    ASSERT_EQ(asset.scenes.size(), 1u);
    ASSERT_EQ(asset.nodes.size(), 1u);
    ASSERT_EQ(asset.meshes.size(), 1u);
    ASSERT_EQ(asset.meshes[0].primitives.size(), 1u);
    EXPECT_TRUE(asset.meshes[0].primitives[0].indicesAccessor.has_value() == false);
    EXPECT_EQ(asset.meshes[0].primitives[0].findAttribute("POSITION")->accessorIndex, 0u);
    ASSERT_EQ(asset.accessors.size(), 1u);
    EXPECT_EQ(asset.accessors[0].count, 3u);
}

// ==========================================================================
// Smooth-normal generation — runs in the loader when a glTF primitive omits
// the NORMAL attribute (Fox.gltf, etc.). Verifies the algorithm directly so
// we don't need a GPU-backed Resources to exercise it.
// ==========================================================================

TEST(GenerateSmoothNormals, SingleTriangleProducesFaceNormal)
{
    // Triangle in XY plane, CCW when viewed from +Z. Face normal is +Z.
    std::vector<Vec3> positions{
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    std::vector<uint32_t> indices{0, 1, 2};

    const auto normals = GltfLoader::generateSmoothNormals(positions, indices);
    ASSERT_EQ(normals.size(), 3u);
    for (const auto& n : normals)
    {
        EXPECT_NEAR(n.x(), 0.0f, 1e-5f);
        EXPECT_NEAR(n.y(), 0.0f, 1e-5f);
        EXPECT_NEAR(n.z(), 1.0f, 1e-5f);
    }
}

TEST(GenerateSmoothNormals, SharedVertexAreaWeightedAverage)
{
    // Two triangles sharing vertex 0 at the origin. Both normals are +Z, so
    // the shared vertex's accumulated normal is also +Z, unit length.
    std::vector<Vec3> positions{
        {0.0f, 0.0f, 0.0f}, // shared
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
    };
    std::vector<uint32_t> indices{
        0, 1, 2, // CCW from +Z, normal +Z
        0, 2, 3, // CCW from +Z, normal +Z
    };

    const auto normals = GltfLoader::generateSmoothNormals(positions, indices);
    ASSERT_EQ(normals.size(), 4u);
    EXPECT_NEAR(normals[0].z(), 1.0f, 1e-5f);
    EXPECT_NEAR(std::sqrt(normals[0].x() * normals[0].x() + normals[0].y() * normals[0].y() +
                          normals[0].z() * normals[0].z()),
                1.0f, 1e-5f);
}

TEST(GenerateSmoothNormals, UnreferencedVertexFallsBackToUp)
{
    // Three real triangle verts plus a stray fourth that no triangle uses.
    // The stray's accumulated normal is zero; the function must not divide
    // by zero and instead emits the documented up-pointing fallback.
    std::vector<Vec3> positions{
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {99.0f, 99.0f, 99.0f}, // unreferenced
    };
    std::vector<uint32_t> indices{0, 1, 2};

    const auto normals = GltfLoader::generateSmoothNormals(positions, indices);
    ASSERT_EQ(normals.size(), 4u);
    EXPECT_NEAR(normals[3].x(), 0.0f, 1e-5f);
    EXPECT_NEAR(normals[3].y(), 1.0f, 1e-5f);
    EXPECT_NEAR(normals[3].z(), 0.0f, 1e-5f);
}

TEST(GenerateSmoothNormals, OutOfRangeIndicesAreSkipped)
{
    // Malformed mesh: an index references a vertex that doesn't exist.
    // Function must skip the bad triangle without UB or crash; remaining
    // triangle still contributes.
    std::vector<Vec3> positions{
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    std::vector<uint32_t> indices{
        0, 1, 2,  // good
        0, 1, 99, // bad — index 99 out of range
    };

    const auto normals = GltfLoader::generateSmoothNormals(positions, indices);
    ASSERT_EQ(normals.size(), 3u);
    for (const auto& n : normals)
    {
        EXPECT_NEAR(n.z(), 1.0f, 1e-5f);
    }
}

// ==========================================================================
// Alpha fields — glTF alphaMode / alphaCutoff / doubleSided must land on the
// engine's Material verbatim so the renderer can route to the right pipeline.
// ==========================================================================

TEST(MaterialAlphaFields, OpaqueIsDefault)
{
    fastgltf::Material gltfMat{};
    Material material;
    applyAlphaFields(gltfMat, material);
    EXPECT_EQ(material.alphaMode(), AlphaMode::Opaque);
    EXPECT_FALSE(material.doubleSided());
    // fastgltf default alphaCutoff is 0.5f per the glTF spec.
    EXPECT_FLOAT_EQ(material.alphaCutoff(), 0.5f);
}

TEST(MaterialAlphaFields, MaskWithCustomCutoff)
{
    fastgltf::Material gltfMat{};
    gltfMat.alphaMode = fastgltf::AlphaMode::Mask;
    gltfMat.alphaCutoff = 0.25f;
    gltfMat.doubleSided = true;
    Material material;
    applyAlphaFields(gltfMat, material);
    EXPECT_EQ(material.alphaMode(), AlphaMode::Mask);
    EXPECT_FLOAT_EQ(material.alphaCutoff(), 0.25f);
    EXPECT_TRUE(material.doubleSided());
}

TEST(MaterialAlphaFields, BlendMapsThrough)
{
    fastgltf::Material gltfMat{};
    gltfMat.alphaMode = fastgltf::AlphaMode::Blend;
    gltfMat.doubleSided = true;
    Material material;
    applyAlphaFields(gltfMat, material);
    EXPECT_EQ(material.alphaMode(), AlphaMode::Blend);
    EXPECT_TRUE(material.doubleSided());
}

// ==========================================================================
// Occlusion strength — glTF spec default is 1.0; explicit values must round
// trip through GltfLoader::loadMaterial onto Material::occlusionStrength().
// ==========================================================================

TEST(MaterialOcclusionStrength, DefaultsToOne)
{
    Material material;
    EXPECT_FLOAT_EQ(material.occlusionStrength(), 1.0f);
}

TEST(MaterialOcclusionStrength, AbsentTextureLeavesStrengthUnchanged)
{
    fastgltf::Material gltfMat{};
    Material material;
    material.occlusionStrength(0.42f);
    applyOcclusionStrength(gltfMat, material);
    EXPECT_FLOAT_EQ(material.occlusionStrength(), 0.42f);
}

TEST(MaterialOcclusionStrength, ExplicitStrengthRoundTrips)
{
    fastgltf::Material gltfMat{};
    gltfMat.occlusionTexture.emplace().strength = 0.5f;
    Material material;
    applyOcclusionStrength(gltfMat, material);
    EXPECT_FLOAT_EQ(material.occlusionStrength(), 0.5f);
}

// ==========================================================================
// KHR_materials_emissive_strength — emissiveFactor is multiplied by the
// extension's strength scalar at load time so HDR emissives reach the bloom
// chain at the authored magnitude.
// ==========================================================================

TEST(EmissiveStrength, DefaultStrengthIsIdentity)
{
    fastgltf::Material gltfMat{};
    gltfMat.emissiveFactor = {0.5f, 0.25f, 0.125f};
    // emissiveStrength defaults to 1.0 per the extension spec.
    Material material;
    applyEmissiveStrength(gltfMat, material);
    EXPECT_FLOAT_EQ(material.emissive().r(), 0.5f);
    EXPECT_FLOAT_EQ(material.emissive().g(), 0.25f);
    EXPECT_FLOAT_EQ(material.emissive().b(), 0.125f);
}

TEST(EmissiveStrength, ExplicitStrengthScalesEmissiveFactor)
{
    fastgltf::Material gltfMat{};
    gltfMat.emissiveFactor = {1.0f, 0.5f, 0.25f};
    gltfMat.emissiveStrength = 4.0f;
    Material material;
    applyEmissiveStrength(gltfMat, material);
    EXPECT_FLOAT_EQ(material.emissive().r(), 4.0f);
    EXPECT_FLOAT_EQ(material.emissive().g(), 2.0f);
    EXPECT_FLOAT_EQ(material.emissive().b(), 1.0f);
}

TEST(EmissiveStrength, ZeroStrengthZeroesEmission)
{
    fastgltf::Material gltfMat{};
    gltfMat.emissiveFactor = {1.0f, 1.0f, 1.0f};
    gltfMat.emissiveStrength = 0.0f;
    Material material;
    applyEmissiveStrength(gltfMat, material);
    EXPECT_FLOAT_EQ(material.emissive().r(), 0.0f);
    EXPECT_FLOAT_EQ(material.emissive().g(), 0.0f);
    EXPECT_FLOAT_EQ(material.emissive().b(), 0.0f);
}

// ==========================================================================
// extensionsRequired — spec MUST refuse to load assets needing extensions we
// don't support. Helper is exposed on GltfLoader so we can hit it without a
// full asset.
// ==========================================================================

TEST(EnsureSupportedExtensions, EmptyVectorAccepts)
{
    std::vector<std::string_view> required;
    EXPECT_NO_THROW(GltfLoader::ensureSupportedExtensions(required));
}

TEST(EnsureSupportedExtensions, SupportedExtensionAccepts)
{
    std::vector<std::string_view> required{"KHR_materials_emissive_strength"};
    EXPECT_NO_THROW(GltfLoader::ensureSupportedExtensions(required));
}

TEST(EnsureSupportedExtensions, UnsupportedExtensionThrows)
{
    std::vector<std::string_view> required{"KHR_draco_mesh_compression"};
    try
    {
        GltfLoader::ensureSupportedExtensions(required);
        FAIL() << "expected ensureSupportedExtensions to throw";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_NE(std::string(e.what()).find("KHR_draco_mesh_compression"), std::string::npos);
    }
}

TEST(EnsureSupportedExtensions, MixedThrowsListingOnlyUnsupported)
{
    std::vector<std::string_view> required{"KHR_materials_emissive_strength",
                                           "KHR_mesh_quantization", "KHR_texture_basisu"};
    try
    {
        GltfLoader::ensureSupportedExtensions(required);
        FAIL() << "expected ensureSupportedExtensions to throw";
    }
    catch (const std::runtime_error& e)
    {
        const std::string what(e.what());
        EXPECT_NE(what.find("KHR_mesh_quantization"), std::string::npos);
        EXPECT_NE(what.find("KHR_texture_basisu"), std::string::npos);
        // The supported one must not be listed in the unsupported message.
        EXPECT_EQ(what.find("KHR_materials_emissive_strength"), std::string::npos);
    }
}

// ==========================================================================
// Primitive mode — vertex layout / index buffer assume triangles. Anything
// else gets skipped (with a warning at load time).
// ==========================================================================

TEST(SupportedPrimitiveType, TrianglesIsSupported)
{
    EXPECT_TRUE(GltfLoader::isSupportedPrimitiveType(fastgltf::PrimitiveType::Triangles));
}

TEST(SupportedPrimitiveType, AllOtherModesAreSkipped)
{
    EXPECT_FALSE(GltfLoader::isSupportedPrimitiveType(fastgltf::PrimitiveType::Points));
    EXPECT_FALSE(GltfLoader::isSupportedPrimitiveType(fastgltf::PrimitiveType::Lines));
    EXPECT_FALSE(GltfLoader::isSupportedPrimitiveType(fastgltf::PrimitiveType::LineLoop));
    EXPECT_FALSE(GltfLoader::isSupportedPrimitiveType(fastgltf::PrimitiveType::LineStrip));
    EXPECT_FALSE(GltfLoader::isSupportedPrimitiveType(fastgltf::PrimitiveType::TriangleStrip));
    EXPECT_FALSE(GltfLoader::isSupportedPrimitiveType(fastgltf::PrimitiveType::TriangleFan));
}

// ==========================================================================
// Per-slot UV-set indices — each glTF TextureInfo names a `texCoord` index
// that the loader must record on Material so the shader samples the right
// vertex stream.
// ==========================================================================

TEST(MaterialTexCoordIndices, AbsentTexturesLeaveDefaultsZero)
{
    fastgltf::Material gltfMat{};
    Material material;
    applyTexCoordIndices(gltfMat, material);
    EXPECT_EQ(material.baseColorTexCoord(), 0);
    EXPECT_EQ(material.emissiveTexCoord(), 0);
    EXPECT_EQ(material.normalTexCoord(), 0);
    EXPECT_EQ(material.metallicRoughnessTexCoord(), 0);
    EXPECT_EQ(material.occlusionTexCoord(), 0);
}

TEST(MaterialTexCoordIndices, ExplicitTexCoordOnesRoundTrip)
{
    fastgltf::Material gltfMat{};
    gltfMat.pbrData.baseColorTexture.emplace().texCoordIndex = 1;
    gltfMat.emissiveTexture.emplace().texCoordIndex = 1;
    gltfMat.normalTexture.emplace().texCoordIndex = 1;
    gltfMat.pbrData.metallicRoughnessTexture.emplace().texCoordIndex = 1;
    gltfMat.occlusionTexture.emplace().texCoordIndex = 1;
    Material material;
    applyTexCoordIndices(gltfMat, material);
    EXPECT_EQ(material.baseColorTexCoord(), 1);
    EXPECT_EQ(material.emissiveTexCoord(), 1);
    EXPECT_EQ(material.normalTexCoord(), 1);
    EXPECT_EQ(material.metallicRoughnessTexCoord(), 1);
    EXPECT_EQ(material.occlusionTexCoord(), 1);
}

TEST(MaterialTexCoordIndices, MixedSlotsRoundTrip)
{
    // Common real-world layout: occlusion baked onto its own UV set.
    fastgltf::Material gltfMat{};
    gltfMat.pbrData.baseColorTexture.emplace().texCoordIndex = 0;
    gltfMat.occlusionTexture.emplace().texCoordIndex = 1;
    Material material;
    applyTexCoordIndices(gltfMat, material);
    EXPECT_EQ(material.baseColorTexCoord(), 0);
    EXPECT_EQ(material.occlusionTexCoord(), 1);
}

// ==========================================================================
// KHR_texture_transform — when present, fastgltf parses uvOffset/uvScale/
// rotation onto TextureInfo; loader copies them onto the per-slot Material
// UvTransform. Absent → identity.
// ==========================================================================

TEST(UvTransformDefault, FieldsAreIdentity)
{
    fire_engine::UvTransform t;
    EXPECT_FLOAT_EQ(t.offsetX, 0.0f);
    EXPECT_FLOAT_EQ(t.offsetY, 0.0f);
    EXPECT_FLOAT_EQ(t.scaleX, 1.0f);
    EXPECT_FLOAT_EQ(t.scaleY, 1.0f);
    EXPECT_FLOAT_EQ(t.rotation, 0.0f);
}

TEST(UvTransformLoader, AbsentExtensionGivesIdentity)
{
    fastgltf::TextureInfo info{};
    auto t = readUvTransform(info);
    EXPECT_FLOAT_EQ(t.offsetX, 0.0f);
    EXPECT_FLOAT_EQ(t.scaleX, 1.0f);
    EXPECT_FLOAT_EQ(t.rotation, 0.0f);
}

TEST(UvTransformLoader, ExplicitTransformRoundTrips)
{
    fastgltf::TextureInfo info{};
    info.transform = std::make_unique<fastgltf::TextureTransform>();
    info.transform->uvOffset = {0.25f, 0.5f};
    info.transform->uvScale = {2.0f, 3.0f};
    info.transform->rotation = 1.5f;
    auto t = readUvTransform(info);
    EXPECT_FLOAT_EQ(t.offsetX, 0.25f);
    EXPECT_FLOAT_EQ(t.offsetY, 0.5f);
    EXPECT_FLOAT_EQ(t.scaleX, 2.0f);
    EXPECT_FLOAT_EQ(t.scaleY, 3.0f);
    EXPECT_FLOAT_EQ(t.rotation, 1.5f);
}

// ==========================================================================
// Vertex colour extraction (mirrors GltfLoader::extractPrimitive)
// ==========================================================================

// Mirrors the per-vertex colour logic: when COLOR_0 data is present, use it;
// when absent, default to white (1,1,1) per the glTF spec.
static fire_engine::Colour3 extractVertexColour(const std::vector<fastgltf::math::fvec4>& colors,
                                                std::size_t index)
{
    if (index < colors.size())
    {
        return {colors[index].x(), colors[index].y(), colors[index].z()};
    }
    return {1.0f, 1.0f, 1.0f};
}

TEST(VertexColourExtraction, DefaultsToWhiteWhenAbsent)
{
    std::vector<fastgltf::math::fvec4> colors;
    auto c = extractVertexColour(colors, 0);
    EXPECT_FLOAT_EQ(c.r(), 1.0f);
    EXPECT_FLOAT_EQ(c.g(), 1.0f);
    EXPECT_FLOAT_EQ(c.b(), 1.0f);
}

TEST(VertexColourExtraction, UsesProvidedColour)
{
    std::vector<fastgltf::math::fvec4> colors;
    colors.push_back(fastgltf::math::fvec4{0.2f, 0.4f, 0.6f, 1.0f});
    auto c = extractVertexColour(colors, 0);
    EXPECT_FLOAT_EQ(c.r(), 0.2f);
    EXPECT_FLOAT_EQ(c.g(), 0.4f);
    EXPECT_FLOAT_EQ(c.b(), 0.6f);
}

TEST(VertexColourExtraction, IndexOutOfRangeFallsBackToWhite)
{
    std::vector<fastgltf::math::fvec4> colors;
    colors.push_back(fastgltf::math::fvec4{1.0f, 0.0f, 0.0f, 1.0f});
    auto c = extractVertexColour(colors, 5);
    EXPECT_FLOAT_EQ(c.r(), 1.0f);
    EXPECT_FLOAT_EQ(c.g(), 1.0f);
    EXPECT_FLOAT_EQ(c.b(), 1.0f);
}

TEST(VertexColourExtraction, IgnoresAlphaChannel)
{
    std::vector<fastgltf::math::fvec4> colors;
    colors.push_back(fastgltf::math::fvec4{0.1f, 0.2f, 0.3f, 0.5f});
    auto c = extractVertexColour(colors, 0);
    EXPECT_FLOAT_EQ(c.r(), 0.1f);
    EXPECT_FLOAT_EQ(c.g(), 0.2f);
    EXPECT_FLOAT_EQ(c.b(), 0.3f);
}
