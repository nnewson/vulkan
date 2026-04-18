#include <gtest/gtest.h>

#include <cstring>

#include <fire_engine/render/constants.hpp>
#include <fire_engine/render/ubo.hpp>

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

TEST(UBO, MorphUBOSize)
{
    EXPECT_EQ(sizeof(MorphUBO) % 16, 0u);
}

TEST(UBO, SkinUBOSize)
{
    EXPECT_EQ(sizeof(SkinUBO) % 16, 0u);
}
