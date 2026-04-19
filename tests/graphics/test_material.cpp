#include <gtest/gtest.h>

#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/texture.hpp>

using namespace fire_engine;

// ---------------------------------------------------------------------------
// Construction and defaults
// ---------------------------------------------------------------------------

TEST(Material, DefaultConstructionHasEmptyName)
{
    Material mat;
    EXPECT_TRUE(mat.name().empty());
}

TEST(Material, DefaultConstructionHasNoTexture)
{
    Material mat;
    EXPECT_FALSE(mat.hasTexture());
}

TEST(Material, DefaultDiffuseIsZero)
{
    Material mat;
    EXPECT_FLOAT_EQ(mat.diffuse().r(), 0.0f);
    EXPECT_FLOAT_EQ(mat.diffuse().g(), 0.0f);
    EXPECT_FLOAT_EQ(mat.diffuse().b(), 0.0f);
}

TEST(Material, DefaultPBRValues)
{
    Material mat;
    EXPECT_FLOAT_EQ(mat.roughness(), 0.0f);
    EXPECT_FLOAT_EQ(mat.metallic(), 0.0f);
    EXPECT_FLOAT_EQ(mat.shininess(), 0.0f);
    EXPECT_FLOAT_EQ(mat.ior(), 0.0f);
    EXPECT_FLOAT_EQ(mat.transparency(), 0.0f);
}

// ---------------------------------------------------------------------------
// Name
// ---------------------------------------------------------------------------

TEST(Material, SetAndGetName)
{
    Material mat;
    mat.name("gold");
    EXPECT_EQ(mat.name(), "gold");
}

// ---------------------------------------------------------------------------
// Colour properties
// ---------------------------------------------------------------------------

TEST(Material, SetAndGetAmbient)
{
    Material mat;
    mat.ambient({0.1f, 0.2f, 0.3f});
    EXPECT_FLOAT_EQ(mat.ambient().r(), 0.1f);
    EXPECT_FLOAT_EQ(mat.ambient().g(), 0.2f);
    EXPECT_FLOAT_EQ(mat.ambient().b(), 0.3f);
}

TEST(Material, SetAndGetDiffuse)
{
    Material mat;
    mat.diffuse({0.8f, 0.6f, 0.4f});
    EXPECT_FLOAT_EQ(mat.diffuse().r(), 0.8f);
    EXPECT_FLOAT_EQ(mat.diffuse().g(), 0.6f);
    EXPECT_FLOAT_EQ(mat.diffuse().b(), 0.4f);
}

TEST(Material, SetAndGetSpecular)
{
    Material mat;
    mat.specular({1.0f, 1.0f, 1.0f});
    EXPECT_FLOAT_EQ(mat.specular().r(), 1.0f);
}

TEST(Material, SetAndGetEmissive)
{
    Material mat;
    mat.emissive({0.5f, 0.0f, 0.0f});
    EXPECT_FLOAT_EQ(mat.emissive().r(), 0.5f);
    EXPECT_FLOAT_EQ(mat.emissive().g(), 0.0f);
}

// ---------------------------------------------------------------------------
// Float properties
// ---------------------------------------------------------------------------

TEST(Material, SetAndGetRoughness)
{
    Material mat;
    mat.roughness(0.75f);
    EXPECT_FLOAT_EQ(mat.roughness(), 0.75f);
}

TEST(Material, SetAndGetMetallic)
{
    Material mat;
    mat.metallic(1.0f);
    EXPECT_FLOAT_EQ(mat.metallic(), 1.0f);
}

TEST(Material, SetAndGetShininess)
{
    Material mat;
    mat.shininess(128.0f);
    EXPECT_FLOAT_EQ(mat.shininess(), 128.0f);
}

TEST(Material, SetAndGetIor)
{
    Material mat;
    mat.ior(1.5f);
    EXPECT_FLOAT_EQ(mat.ior(), 1.5f);
}

TEST(Material, SetAndGetTransparency)
{
    Material mat;
    mat.transparency(0.5f);
    EXPECT_FLOAT_EQ(mat.transparency(), 0.5f);
}

TEST(Material, SetAndGetIllum)
{
    Material mat;
    mat.illum(2);
    EXPECT_EQ(mat.illum(), 2);
}

TEST(Material, SetAndGetSheen)
{
    Material mat;
    mat.sheen(0.3f);
    EXPECT_FLOAT_EQ(mat.sheen(), 0.3f);
}

TEST(Material, SetAndGetClearcoat)
{
    Material mat;
    mat.clearcoat(0.8f);
    EXPECT_FLOAT_EQ(mat.clearcoat(), 0.8f);
}

TEST(Material, SetAndGetClearcoatRoughness)
{
    Material mat;
    mat.clearcoatRoughness(0.1f);
    EXPECT_FLOAT_EQ(mat.clearcoatRoughness(), 0.1f);
}

TEST(Material, SetAndGetAnisotropy)
{
    Material mat;
    mat.anisotropy(0.5f);
    EXPECT_FLOAT_EQ(mat.anisotropy(), 0.5f);
}

TEST(Material, SetAndGetAnisotropyRotation)
{
    Material mat;
    mat.anisotropyRotation(1.57f);
    EXPECT_FLOAT_EQ(mat.anisotropyRotation(), 1.57f);
}

// ---------------------------------------------------------------------------
// Alpha mode
// ---------------------------------------------------------------------------

TEST(Material, DefaultAlphaModeIsOpaque)
{
    Material mat;
    EXPECT_EQ(mat.alphaMode(), AlphaMode::Opaque);
}

TEST(Material, DefaultAlphaCutoffMatchesGltfSpec)
{
    Material mat;
    EXPECT_FLOAT_EQ(mat.alphaCutoff(), 0.5f);
}

TEST(Material, DefaultDoubleSidedIsFalse)
{
    Material mat;
    EXPECT_FALSE(mat.doubleSided());
}

TEST(Material, SetAndGetAlphaMode)
{
    Material mat;
    mat.alphaMode(AlphaMode::Mask);
    EXPECT_EQ(mat.alphaMode(), AlphaMode::Mask);
    mat.alphaMode(AlphaMode::Blend);
    EXPECT_EQ(mat.alphaMode(), AlphaMode::Blend);
}

TEST(Material, SetAndGetAlphaCutoff)
{
    Material mat;
    mat.alphaCutoff(0.25f);
    EXPECT_FLOAT_EQ(mat.alphaCutoff(), 0.25f);
}

TEST(Material, SetAndGetDoubleSided)
{
    Material mat;
    mat.doubleSided(true);
    EXPECT_TRUE(mat.doubleSided());
}

TEST(Material, SetAndGetMapKd)
{
    Material mat;
    mat.mapKd("textures/brick.png");
    EXPECT_EQ(mat.mapKd(), "textures/brick.png");
}

// ---------------------------------------------------------------------------
// Texture pointer
// ---------------------------------------------------------------------------

TEST(Material, TexturePointerAssignment)
{
    Material mat;
    Texture tex;
    mat.texture(&tex);
    EXPECT_TRUE(mat.hasTexture());
}

TEST(Material, TexturePointerNullAfterDefault)
{
    Material mat;
    EXPECT_FALSE(mat.hasTexture());
}

// ---------------------------------------------------------------------------
// Emissive texture pointer
// ---------------------------------------------------------------------------

TEST(Material, DefaultConstructionHasNoEmissiveTexture)
{
    Material mat;
    EXPECT_FALSE(mat.hasEmissiveTexture());
}

TEST(Material, EmissiveTexturePointerAssignment)
{
    Material mat;
    Texture tex;
    mat.emissiveTexture(&tex);
    EXPECT_TRUE(mat.hasEmissiveTexture());
}

// ---------------------------------------------------------------------------
// Normal texture pointer
// ---------------------------------------------------------------------------

TEST(Material, DefaultConstructionHasNoNormalTexture)
{
    Material mat;
    EXPECT_FALSE(mat.hasNormalTexture());
}

TEST(Material, NormalTexturePointerAssignment)
{
    Material mat;
    Texture tex;
    mat.normalTexture(&tex);
    EXPECT_TRUE(mat.hasNormalTexture());
}

// ---------------------------------------------------------------------------
// MetallicRoughness texture pointer
// ---------------------------------------------------------------------------

TEST(Material, DefaultConstructionHasNoMetallicRoughnessTexture)
{
    Material mat;
    EXPECT_FALSE(mat.hasMetallicRoughnessTexture());
}

TEST(Material, MetallicRoughnessTexturePointerAssignment)
{
    Material mat;
    Texture tex;
    mat.metallicRoughnessTexture(&tex);
    EXPECT_TRUE(mat.hasMetallicRoughnessTexture());
}

TEST(Material, OcclusionTextureDefaultsToNull)
{
    Material mat;
    EXPECT_FALSE(mat.hasOcclusionTexture());
}

TEST(Material, OcclusionTexturePointerAssignment)
{
    Material mat;
    Texture tex;
    mat.occlusionTexture(&tex);
    EXPECT_TRUE(mat.hasOcclusionTexture());
}

// ---------------------------------------------------------------------------
// Copy semantics
// ---------------------------------------------------------------------------

TEST(Material, CopyConstructionCopiesAllProperties)
{
    Texture baseTex, emissiveTex, normalTex, mrTex, occTex;
    Material original;
    original.name("copper");
    original.diffuse({0.7f, 0.3f, 0.1f});
    original.roughness(0.4f);
    original.metallic(1.0f);
    original.texture(&baseTex);
    original.emissiveTexture(&emissiveTex);
    original.normalTexture(&normalTex);
    original.metallicRoughnessTexture(&mrTex);
    original.occlusionTexture(&occTex);

    Material copy(original);
    EXPECT_EQ(copy.name(), "copper");
    EXPECT_FLOAT_EQ(copy.diffuse().r(), 0.7f);
    EXPECT_FLOAT_EQ(copy.roughness(), 0.4f);
    EXPECT_FLOAT_EQ(copy.metallic(), 1.0f);
    EXPECT_TRUE(copy.hasTexture());
    EXPECT_TRUE(copy.hasEmissiveTexture());
    EXPECT_TRUE(copy.hasNormalTexture());
    EXPECT_TRUE(copy.hasMetallicRoughnessTexture());
    EXPECT_TRUE(copy.hasOcclusionTexture());
}

TEST(Material, CopyAssignmentCopiesAllProperties)
{
    Material original;
    original.name("silver");
    original.roughness(0.2f);

    Material copy;
    copy = original;
    EXPECT_EQ(copy.name(), "silver");
    EXPECT_FLOAT_EQ(copy.roughness(), 0.2f);
}

// ---------------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------------

TEST(Material, MoveConstructionTransfersState)
{
    Material original;
    original.name("glass");
    original.transparency(0.9f);

    Material moved(std::move(original));
    EXPECT_EQ(moved.name(), "glass");
    EXPECT_FLOAT_EQ(moved.transparency(), 0.9f);
}

TEST(Material, MoveAssignmentTransfersState)
{
    Material original;
    original.name("wood");

    Material target;
    target = std::move(original);
    EXPECT_EQ(target.name(), "wood");
}

// ---------------------------------------------------------------------------
// Type traits
// ---------------------------------------------------------------------------

TEST(Material, IsCopyable)
{
    EXPECT_TRUE(std::is_copy_constructible_v<Material>);
    EXPECT_TRUE(std::is_copy_assignable_v<Material>);
}

TEST(Material, IsMovable)
{
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<Material>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<Material>);
}
