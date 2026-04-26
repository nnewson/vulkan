#include <gtest/gtest.h>

#include <cstring>

#include <fire_engine/render/constants.hpp>
#include <fire_engine/render/ubo.hpp>

using fire_engine::EnvironmentCaptureUBO;
using fire_engine::LightUBO;
using fire_engine::MaterialUBO;
using fire_engine::MorphUBO;
using fire_engine::ShadowUBO;
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
    EXPECT_EQ(ubo.textureFlags[0], 0);
}

TEST(UBO, MaterialUBOFieldOrder)
{
    static_assert(offsetof(MaterialUBO, diffuseAlpha) < offsetof(MaterialUBO, emissiveRoughness),
                  "diffuseAlpha must precede emissiveRoughness to match shader layout");
    static_assert(offsetof(MaterialUBO, emissiveRoughness) < offsetof(MaterialUBO, materialParams),
                  "emissiveRoughness must precede materialParams to match shader layout");
    static_assert(offsetof(MaterialUBO, materialParams) < offsetof(MaterialUBO, textureFlags),
                  "materialParams must precede textureFlags to match shader layout");
    static_assert(offsetof(MaterialUBO, extraFlags) < offsetof(MaterialUBO, texCoordIndices),
                  "extraFlags must precede texCoordIndices to match shader layout");
    static_assert(offsetof(MaterialUBO, textureFlags) < offsetof(MaterialUBO, extraFlags),
                  "textureFlags must precede extraFlags to match shader layout");
    SUCCEED();
}

TEST(UBO, MaterialUBOAlphaCutoffRoundTrip)
{
    MaterialUBO ubo{};
    ubo.materialParams[2] = 0.25f;
    EXPECT_FLOAT_EQ(ubo.materialParams[2], 0.25f);
}

TEST(UBO, MaterialUBOAlphaRoundTrip)
{
    MaterialUBO ubo{};
    ubo.diffuseAlpha[3] = 0.75f;
    EXPECT_FLOAT_EQ(ubo.diffuseAlpha[3], 0.75f);
}

TEST(UBO, MaterialUBOHasTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.textureFlags[0] = 1;
    EXPECT_EQ(ubo.textureFlags[0], 1);
}

TEST(UBO, MaterialUBOTexCoordIndicesDefaultToZero)
{
    MaterialUBO ubo{};
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(ubo.texCoordIndices[i], 0);
    }
    // Occlusion's UV-set index also lives in extraFlags.y.
    EXPECT_EQ(ubo.extraFlags[1], 0);
}

TEST(UBO, MaterialUBOTexCoordIndicesRoundTrip)
{
    MaterialUBO ubo{};
    ubo.texCoordIndices[0] = 1; // baseColor on TEXCOORD_1
    ubo.texCoordIndices[1] = 0;
    ubo.texCoordIndices[2] = 1; // normal on TEXCOORD_1
    ubo.texCoordIndices[3] = 0;
    ubo.extraFlags[1] = 1;       // occlusion on TEXCOORD_1
    EXPECT_EQ(ubo.texCoordIndices[0], 1);
    EXPECT_EQ(ubo.texCoordIndices[2], 1);
    EXPECT_EQ(ubo.extraFlags[1], 1);
}

TEST(UBO, MaterialUBOHasEmissiveTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.textureFlags[1] = 1;
    EXPECT_EQ(ubo.textureFlags[1], 1);
}

TEST(UBO, MaterialUBOHasNormalTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.textureFlags[2] = 1;
    EXPECT_EQ(ubo.textureFlags[2], 1);
}

TEST(UBO, MaterialUBOHasMetallicRoughnessTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.textureFlags[3] = 1;
    EXPECT_EQ(ubo.textureFlags[3], 1);
}

TEST(UBO, MaterialUBOHasOcclusionTextureCanBeSet)
{
    MaterialUBO ubo{};
    ubo.extraFlags[0] = 1;
    EXPECT_EQ(ubo.extraFlags[0], 1);
}

TEST(UBO, MaterialUBOTextureFlagsFieldOrder)
{
    static_assert(offsetof(MaterialUBO, textureFlags) % 16 == 0,
                  "textureFlags must be 16-byte aligned for std140 ivec4");
    static_assert(offsetof(MaterialUBO, extraFlags) % 16 == 0,
                  "extraFlags must be 16-byte aligned for std140 ivec4");
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

TEST(UBO, EnvironmentCaptureUBOSize)
{
    EXPECT_EQ(sizeof(EnvironmentCaptureUBO) % 16, 0u);
}

TEST(UBO, EnvironmentCaptureUBODefaultFaceIndexIsZero)
{
    EnvironmentCaptureUBO ubo{};
    EXPECT_EQ(ubo.faceIndex, 0);
    EXPECT_EQ(ubo.faceExtent, 0);
}

TEST(UBO, LightUBODefaults)
{
    LightUBO ubo{};
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ(ubo.direction[i], 0.0f);
        EXPECT_FLOAT_EQ(ubo.colour[i], 0.0f);
        EXPECT_FLOAT_EQ(ubo.iblParams[i], 0.0f);
        EXPECT_FLOAT_EQ(ubo.shadowParams[i], 0.0f);
        EXPECT_FLOAT_EQ(ubo.environmentParams[i], 0.0f);
    }
}

TEST(UBO, LightUBOFieldOrder)
{
    static_assert(offsetof(LightUBO, direction) < offsetof(LightUBO, colour),
                  "direction must precede colour to match shader layout");
    static_assert(offsetof(LightUBO, colour) < offsetof(LightUBO, cascadeViewProj),
                  "colour must precede cascadeViewProj to match shader layout");
    static_assert(offsetof(LightUBO, cascadeViewProj) < offsetof(LightUBO, cascadeSplits),
                  "cascadeViewProj must precede cascadeSplits to match shader layout");
    static_assert(offsetof(LightUBO, cascadeSplits) < offsetof(LightUBO, iblParams),
                  "cascadeSplits must precede iblParams to match shader layout");
    static_assert(offsetof(LightUBO, iblParams) < offsetof(LightUBO, shadowParams),
                  "iblParams must precede shadowParams to match shader layout");
    static_assert(offsetof(LightUBO, shadowParams) < offsetof(LightUBO, environmentParams),
                  "shadowParams must precede environmentParams to match shader layout");
    SUCCEED();
}

TEST(UBO, LightUBODirectionAligned16)
{
    static_assert(offsetof(LightUBO, direction) % 16 == 0,
                  "direction must be 16-byte aligned for std140 vec4");
    static_assert(offsetof(LightUBO, colour) % 16 == 0,
                  "colour must be 16-byte aligned for std140 vec4");
    static_assert(offsetof(LightUBO, cascadeViewProj) % 16 == 0,
                  "cascadeViewProj must be 16-byte aligned for std140 mat4[]");
    static_assert(offsetof(LightUBO, cascadeSplits) % 16 == 0,
                  "cascadeSplits must be 16-byte aligned for std140 vec4");
    static_assert(offsetof(LightUBO, iblParams) % 16 == 0,
                  "iblParams must be 16-byte aligned for std140 vec4");
    static_assert(offsetof(LightUBO, shadowParams) % 16 == 0,
                  "shadowParams must be 16-byte aligned for std140 vec4");
    static_assert(offsetof(LightUBO, environmentParams) % 16 == 0,
                  "environmentParams must be 16-byte aligned for std140 vec4");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// ShadowUBO structure tests
// ---------------------------------------------------------------------------

TEST(UBO, ShadowUBOSize)
{
    EXPECT_EQ(sizeof(ShadowUBO) % 16, 0u);
}

TEST(UBO, ShadowUBODefaultHasSkinIsZero)
{
    ShadowUBO ubo{};
    EXPECT_EQ(ubo.hasSkin, 0);
}

TEST(UBO, ShadowUBOHasSkinCanBeSet)
{
    ShadowUBO ubo{};
    ubo.hasSkin = 1;
    EXPECT_EQ(ubo.hasSkin, 1);
}

TEST(UBO, ShadowUBOFieldOrder)
{
    static_assert(offsetof(ShadowUBO, model) < offsetof(ShadowUBO, lightViewProj),
                  "model must precede lightViewProj to match shader layout");
    static_assert(offsetof(ShadowUBO, lightViewProj) < offsetof(ShadowUBO, hasSkin),
                  "lightViewProj must precede hasSkin to match shader layout");
    SUCCEED();
}

TEST(UBO, ShadowUBOMatricesAligned16)
{
    static_assert(offsetof(ShadowUBO, model) % 16 == 0,
                  "model must be 16-byte aligned for std140 mat4");
    static_assert(offsetof(ShadowUBO, lightViewProj) % 16 == 0,
                  "lightViewProj must be 16-byte aligned for std140 mat4");
    SUCCEED();
}
