#include <gtest/gtest.h>

#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/texture.hpp>

using namespace fire_engine;

TEST(Material, DefaultConstructionUsesPbrDefaults)
{
    Material mat;

    EXPECT_TRUE(mat.name().empty());
    EXPECT_FALSE(mat.hasTexture());
    EXPECT_FALSE(mat.hasEmissiveTexture());
    EXPECT_FALSE(mat.hasNormalTexture());
    EXPECT_FALSE(mat.hasMetallicRoughnessTexture());
    EXPECT_FALSE(mat.hasOcclusionTexture());

    EXPECT_FLOAT_EQ(mat.diffuse().r(), 0.0f);
    EXPECT_FLOAT_EQ(mat.diffuse().g(), 0.0f);
    EXPECT_FLOAT_EQ(mat.diffuse().b(), 0.0f);
    EXPECT_FLOAT_EQ(mat.emissive().r(), 0.0f);
    EXPECT_FLOAT_EQ(mat.roughness(), 0.0f);
    EXPECT_FLOAT_EQ(mat.metallic(), 0.0f);
    EXPECT_FLOAT_EQ(mat.alpha(), 1.0f);
    EXPECT_FLOAT_EQ(mat.normalScale(), 1.0f);
    EXPECT_EQ(mat.alphaMode(), AlphaMode::Opaque);
    EXPECT_FLOAT_EQ(mat.alphaCutoff(), 0.5f);
    EXPECT_FALSE(mat.doubleSided());
}

TEST(Material, SetAndGetCorePbrFields)
{
    Material mat;

    mat.name("helmet");
    mat.diffuse({0.8f, 0.6f, 0.4f});
    mat.emissive({0.1f, 0.2f, 0.3f});
    mat.roughness(0.75f);
    mat.metallic(0.5f);
    mat.alpha(0.25f);
    mat.normalScale(0.9f);
    mat.alphaMode(AlphaMode::Blend);
    mat.alphaCutoff(0.2f);
    mat.doubleSided(true);

    EXPECT_EQ(mat.name(), "helmet");
    EXPECT_FLOAT_EQ(mat.diffuse().r(), 0.8f);
    EXPECT_FLOAT_EQ(mat.diffuse().g(), 0.6f);
    EXPECT_FLOAT_EQ(mat.diffuse().b(), 0.4f);
    EXPECT_FLOAT_EQ(mat.emissive().r(), 0.1f);
    EXPECT_FLOAT_EQ(mat.emissive().g(), 0.2f);
    EXPECT_FLOAT_EQ(mat.emissive().b(), 0.3f);
    EXPECT_FLOAT_EQ(mat.roughness(), 0.75f);
    EXPECT_FLOAT_EQ(mat.metallic(), 0.5f);
    EXPECT_FLOAT_EQ(mat.alpha(), 0.25f);
    EXPECT_FLOAT_EQ(mat.normalScale(), 0.9f);
    EXPECT_EQ(mat.alphaMode(), AlphaMode::Blend);
    EXPECT_FLOAT_EQ(mat.alphaCutoff(), 0.2f);
    EXPECT_TRUE(mat.doubleSided());
}

TEST(Material, TexturePointersRoundTrip)
{
    Material mat;
    Texture base;
    Texture emissive;
    Texture normal;
    Texture mr;
    Texture occlusion;

    mat.texture(&base);
    mat.emissiveTexture(&emissive);
    mat.normalTexture(&normal);
    mat.metallicRoughnessTexture(&mr);
    mat.occlusionTexture(&occlusion);

    EXPECT_TRUE(mat.hasTexture());
    EXPECT_TRUE(mat.hasEmissiveTexture());
    EXPECT_TRUE(mat.hasNormalTexture());
    EXPECT_TRUE(mat.hasMetallicRoughnessTexture());
    EXPECT_TRUE(mat.hasOcclusionTexture());
}

TEST(Material, MoveConstructionPreservesCoreFields)
{
    Material original;
    original.name("moved");
    original.diffuse({0.2f, 0.4f, 0.6f});
    original.emissive({0.7f, 0.1f, 0.0f});
    original.roughness(0.8f);
    original.metallic(0.9f);
    original.alpha(0.3f);
    original.normalScale(0.6f);

    Material moved(std::move(original));

    EXPECT_EQ(moved.name(), "moved");
    EXPECT_FLOAT_EQ(moved.diffuse().r(), 0.2f);
    EXPECT_FLOAT_EQ(moved.emissive().r(), 0.7f);
    EXPECT_FLOAT_EQ(moved.roughness(), 0.8f);
    EXPECT_FLOAT_EQ(moved.metallic(), 0.9f);
    EXPECT_FLOAT_EQ(moved.alpha(), 0.3f);
    EXPECT_FLOAT_EQ(moved.normalScale(), 0.6f);
}
