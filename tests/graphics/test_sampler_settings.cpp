#include <fastgltf/types.hpp>

#include <fire_engine/graphics/sampler_settings.hpp>

#include <gtest/gtest.h>

using fire_engine::FilterMode;
using fire_engine::SamplerSettings;
using fire_engine::WrapMode;

// ==========================================================================
// SamplerSettings defaults
// ==========================================================================

TEST(SamplerSettings, DefaultsToRepeatAndLinear)
{
    SamplerSettings s;
    EXPECT_EQ(s.wrapS, WrapMode::Repeat);
    EXPECT_EQ(s.wrapT, WrapMode::Repeat);
    EXPECT_EQ(s.magFilter, FilterMode::Linear);
    EXPECT_EQ(s.minFilter, FilterMode::Linear);
}

TEST(SamplerSettings, EqualityOperator)
{
    SamplerSettings a;
    SamplerSettings b;
    EXPECT_EQ(a, b);

    b.wrapS = WrapMode::ClampToEdge;
    EXPECT_NE(a, b);
}

TEST(SamplerSettings, AllWrapModesDistinct)
{
    EXPECT_NE(WrapMode::Repeat, WrapMode::MirroredRepeat);
    EXPECT_NE(WrapMode::Repeat, WrapMode::ClampToEdge);
    EXPECT_NE(WrapMode::MirroredRepeat, WrapMode::ClampToEdge);
}

TEST(SamplerSettings, AllFilterModesDistinct)
{
    EXPECT_NE(FilterMode::Nearest, FilterMode::Linear);
}

// ==========================================================================
// Wrap/Filter mapping (mirrors GltfLoader helper logic)
// ==========================================================================

static WrapMode toWrapMode(fastgltf::Wrap w)
{
    switch (w)
    {
    case fastgltf::Wrap::MirroredRepeat:
        return WrapMode::MirroredRepeat;
    case fastgltf::Wrap::ClampToEdge:
        return WrapMode::ClampToEdge;
    default:
        return WrapMode::Repeat;
    }
}

static FilterMode toFilterMode(fastgltf::Filter f)
{
    switch (f)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return FilterMode::Nearest;
    default:
        return FilterMode::Linear;
    }
}

TEST(SamplerMapping, WrapRepeat)
{
    EXPECT_EQ(toWrapMode(fastgltf::Wrap::Repeat), WrapMode::Repeat);
}

TEST(SamplerMapping, WrapMirroredRepeat)
{
    EXPECT_EQ(toWrapMode(fastgltf::Wrap::MirroredRepeat), WrapMode::MirroredRepeat);
}

TEST(SamplerMapping, WrapClampToEdge)
{
    EXPECT_EQ(toWrapMode(fastgltf::Wrap::ClampToEdge), WrapMode::ClampToEdge);
}

TEST(SamplerMapping, FilterNearest)
{
    EXPECT_EQ(toFilterMode(fastgltf::Filter::Nearest), FilterMode::Nearest);
}

TEST(SamplerMapping, FilterLinear)
{
    EXPECT_EQ(toFilterMode(fastgltf::Filter::Linear), FilterMode::Linear);
}

TEST(SamplerMapping, FilterNearestMipMapVariants)
{
    EXPECT_EQ(toFilterMode(fastgltf::Filter::NearestMipMapNearest), FilterMode::Nearest);
    EXPECT_EQ(toFilterMode(fastgltf::Filter::NearestMipMapLinear), FilterMode::Nearest);
}

TEST(SamplerMapping, FilterLinearMipMapVariants)
{
    EXPECT_EQ(toFilterMode(fastgltf::Filter::LinearMipMapNearest), FilterMode::Linear);
    EXPECT_EQ(toFilterMode(fastgltf::Filter::LinearMipMapLinear), FilterMode::Linear);
}
