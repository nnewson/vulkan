#include <gtest/gtest.h>

#include <cstring>

#include <fire_engine/render/constants.hpp>
#include <fire_engine/render/ubo.hpp>

using fire_engine::LightUBO;
using fire_engine::MaterialUBO;
using fire_engine::MorphUBO;
using fire_engine::SkinUBO;
using fire_engine::UniformBufferObject;

// ---------------------------------------------------------------------------
// MorphUBO structure tests
// ---------------------------------------------------------------------------

TEST(MorphUBO, DefaultInitialisation)
{
    MorphUBO ubo{};
    EXPECT_EQ(ubo.hasMorph, 0);
    EXPECT_EQ(ubo.morphTargetCount, 0);
    EXPECT_EQ(ubo.vertexCount, 0);
    for (int i = 0; i < fire_engine::MAX_MORPH_TARGETS; ++i)
    {
        EXPECT_FLOAT_EQ(ubo.weights[i], 0.0f);
    }
}

TEST(MorphUBO, MaxMorphTargetsConstant)
{
    EXPECT_GE(fire_engine::MAX_MORPH_TARGETS, 2);
    EXPECT_EQ(fire_engine::MAX_MORPH_TARGETS, 8);
}

TEST(MorphUBO, WeightsArraySize)
{
    MorphUBO ubo{};
    EXPECT_EQ(sizeof(ubo.weights) / sizeof(float),
              static_cast<std::size_t>(fire_engine::MAX_MORPH_TARGETS));
}

TEST(MorphUBO, SetWeights)
{
    MorphUBO ubo{};
    ubo.hasMorph = 1;
    ubo.morphTargetCount = 2;
    ubo.vertexCount = 24;
    ubo.weights[0] = 0.5f;
    ubo.weights[1] = 0.8f;

    EXPECT_EQ(ubo.hasMorph, 1);
    EXPECT_EQ(ubo.morphTargetCount, 2);
    EXPECT_EQ(ubo.vertexCount, 24);
    EXPECT_FLOAT_EQ(ubo.weights[0], 0.5f);
    EXPECT_FLOAT_EQ(ubo.weights[1], 0.8f);
}

// ---------------------------------------------------------------------------
// UBO alignment sanity
// ---------------------------------------------------------------------------

TEST(UBO, UniformBufferObjectSize)
{
    // Must be compatible with std140 layout
    EXPECT_EQ(sizeof(UniformBufferObject) % 16, 0u);
}

TEST(UBO, MaterialUBOSize)
{
    EXPECT_EQ(sizeof(MaterialUBO) % 16, 0u);
}

TEST(UBO, MaterialUBOHasTextureDefaultsToZero)
{
    MaterialUBO ubo{};
    EXPECT_EQ(ubo.hasTexture, 0);
}

TEST(UBO, MaterialUBOAlphaCutoffSitsBeforeHasTexture)
{
    static_assert(offsetof(MaterialUBO, alphaCutoff) < offsetof(MaterialUBO, hasTexture),
                  "alphaCutoff must precede hasTexture to match shader layout");
    static_assert(offsetof(MaterialUBO, anisotropyRotation) <
                      offsetof(MaterialUBO, alphaCutoff),
                  "alphaCutoff must sit after anisotropyRotation to match shader layout");
    SUCCEED();
}

TEST(UBO, MaterialUBOAlphaCutoffRoundTrip)
{
    MaterialUBO ubo{};
    ubo.alphaCutoff = 0.25f;
    EXPECT_FLOAT_EQ(ubo.alphaCutoff, 0.25f);
}

TEST(UBO, MaterialUBOHasTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.hasTexture = 1;
    EXPECT_EQ(ubo.hasTexture, 1);
}

TEST(UBO, MaterialUBOHasEmissiveTextureDefaultsToZero)
{
    MaterialUBO ubo{};
    EXPECT_EQ(ubo.hasEmissiveTexture, 0);
}

TEST(UBO, MaterialUBOHasEmissiveTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.hasEmissiveTexture = 1;
    EXPECT_EQ(ubo.hasEmissiveTexture, 1);
}

TEST(UBO, MaterialUBOHasNormalTextureDefaultsToZero)
{
    MaterialUBO ubo{};
    EXPECT_EQ(ubo.hasNormalTexture, 0);
}

TEST(UBO, MaterialUBOHasNormalTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.hasNormalTexture = 1;
    EXPECT_EQ(ubo.hasNormalTexture, 1);
}

TEST(UBO, MaterialUBOHasMetallicRoughnessTextureDefaultsToZero)
{
    MaterialUBO ubo{};
    EXPECT_EQ(ubo.hasMetallicRoughnessTexture, 0);
}

TEST(UBO, MaterialUBOHasMetallicRoughnessTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.hasMetallicRoughnessTexture = 1;
    EXPECT_EQ(ubo.hasMetallicRoughnessTexture, 1);
}

TEST(UBO, MaterialUBOHasOcclusionTextureDefaultsToZero)
{
    MaterialUBO ubo{};
    EXPECT_EQ(ubo.hasOcclusionTexture, 0);
}

TEST(UBO, MaterialUBOHasOcclusionTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.hasOcclusionTexture = 1;
    EXPECT_EQ(ubo.hasOcclusionTexture, 1);
}

TEST(UBO, MaterialUBOTextureFlagsFieldOrder)
{
    static_assert(offsetof(MaterialUBO, hasTexture) < offsetof(MaterialUBO, hasEmissiveTexture),
                  "hasTexture must precede hasEmissiveTexture to match shader layout");
    static_assert(offsetof(MaterialUBO, hasEmissiveTexture) <
                      offsetof(MaterialUBO, hasNormalTexture),
                  "hasEmissiveTexture must precede hasNormalTexture to match shader layout");
    static_assert(offsetof(MaterialUBO, hasNormalTexture) <
                      offsetof(MaterialUBO, hasMetallicRoughnessTexture),
                  "hasNormalTexture must precede hasMetallicRoughnessTexture to match shader "
                  "layout");
    static_assert(offsetof(MaterialUBO, hasMetallicRoughnessTexture) <
                      offsetof(MaterialUBO, hasOcclusionTexture),
                  "hasMetallicRoughnessTexture must precede hasOcclusionTexture to match shader "
                  "layout");
    SUCCEED();
}

TEST(UBO, MorphUBOSize)
{
    EXPECT_EQ(sizeof(MorphUBO) % 16, 0u);
}

TEST(UBO, SkinUBOSize)
{
    EXPECT_EQ(sizeof(SkinUBO) % 16, 0u);
}

TEST(UBO, LightUBOSize)
{
    EXPECT_EQ(sizeof(LightUBO) % 16, 0u);
}

TEST(UBO, LightUBODefaults)
{
    LightUBO ubo{};
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ(ubo.direction[i], 0.0f);
        EXPECT_FLOAT_EQ(ubo.colour[i], 0.0f);
    }
}

TEST(UBO, LightUBOFieldOrder)
{
    static_assert(offsetof(LightUBO, direction) < offsetof(LightUBO, colour),
                  "direction must precede colour to match shader layout");
    static_assert(offsetof(LightUBO, colour) < offsetof(LightUBO, lightViewProj),
                  "colour must precede lightViewProj to match shader layout");
    SUCCEED();
}

TEST(UBO, LightUBODirectionAligned16)
{
    static_assert(offsetof(LightUBO, direction) % 16 == 0,
                  "direction must be 16-byte aligned for std140 vec4");
    static_assert(offsetof(LightUBO, colour) % 16 == 0,
                  "colour must be 16-byte aligned for std140 vec4");
    static_assert(offsetof(LightUBO, lightViewProj) % 16 == 0,
                  "lightViewProj must be 16-byte aligned for std140 mat4");
    SUCCEED();
}
