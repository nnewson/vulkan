#include <fire_engine/graphics/material.hpp>

#include <stdexcept>

#include <gtest/gtest.h>

using fire_engine::Material;

// ==========================================================================
// Default values
// ==========================================================================

TEST(MaterialDefault, HasCorrectDefaults)
{
    Material m;
    EXPECT_TRUE(m.name().empty());
    EXPECT_FLOAT_EQ(m.ambient().r(), 0.2f);
    EXPECT_FLOAT_EQ(m.ambient().g(), 0.2f);
    EXPECT_FLOAT_EQ(m.ambient().b(), 0.2f);
    EXPECT_FLOAT_EQ(m.diffuse().r(), 1.0f);
    EXPECT_FLOAT_EQ(m.diffuse().g(), 1.0f);
    EXPECT_FLOAT_EQ(m.diffuse().b(), 1.0f);
    EXPECT_FLOAT_EQ(m.specular().r(), 0.0f);
    EXPECT_FLOAT_EQ(m.specular().g(), 0.0f);
    EXPECT_FLOAT_EQ(m.specular().b(), 0.0f);
    EXPECT_FLOAT_EQ(m.emissive().r(), 0.0f);
    EXPECT_FLOAT_EQ(m.shininess(), 0.0f);
    EXPECT_FLOAT_EQ(m.ior(), 1.0f);
    EXPECT_FLOAT_EQ(m.transparency(), 0.0f);
    EXPECT_EQ(m.illum(), 0);
    EXPECT_TRUE(m.mapKd().empty());
    EXPECT_FLOAT_EQ(m.roughness(), 1.0f);
    EXPECT_FLOAT_EQ(m.metallic(), 0.0f);
    EXPECT_FLOAT_EQ(m.sheen(), 0.0f);
    EXPECT_FLOAT_EQ(m.clearcoat(), 0.0f);
    EXPECT_FLOAT_EQ(m.clearcoatRoughness(), 0.0f);
    EXPECT_FLOAT_EQ(m.anisotropy(), 0.0f);
    EXPECT_FLOAT_EQ(m.anisotropyRotation(), 0.0f);
}

// ==========================================================================
// Loading — basic properties
// ==========================================================================

TEST(MaterialLoading, LoadsSingleMaterial)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    EXPECT_EQ(materials.size(), 1u);
}

TEST(MaterialLoading, MaterialName)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    EXPECT_EQ(materials.front().name(), "red_material");
}

TEST(MaterialLoading, AmbientColour)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    const auto& m = materials.front();
    EXPECT_FLOAT_EQ(m.ambient().r(), 0.1f);
    EXPECT_FLOAT_EQ(m.ambient().g(), 0.0f);
    EXPECT_FLOAT_EQ(m.ambient().b(), 0.0f);
}

TEST(MaterialLoading, DiffuseColour)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    const auto& m = materials.front();
    EXPECT_FLOAT_EQ(m.diffuse().r(), 1.0f);
    EXPECT_FLOAT_EQ(m.diffuse().g(), 0.0f);
    EXPECT_FLOAT_EQ(m.diffuse().b(), 0.0f);
}

TEST(MaterialLoading, SpecularColour)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    const auto& m = materials.front();
    EXPECT_FLOAT_EQ(m.specular().r(), 0.5f);
    EXPECT_FLOAT_EQ(m.specular().g(), 0.5f);
    EXPECT_FLOAT_EQ(m.specular().b(), 0.5f);
}

TEST(MaterialLoading, Shininess)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    EXPECT_FLOAT_EQ(materials.front().shininess(), 32.0f);
}

TEST(MaterialLoading, IndexOfRefraction)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    EXPECT_FLOAT_EQ(materials.front().ior(), 1.5f);
}

TEST(MaterialLoading, OpacityViaD)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    // d 0.9 → transparency = 1.0 - 0.9 = 0.1
    EXPECT_NEAR(materials.front().transparency(), 0.1f, 1e-6f);
}

TEST(MaterialLoading, IlluminationModel)
{
    auto materials = Material::load_from_file("test_assets/basic.mtl");
    EXPECT_EQ(materials.front().illum(), 2);
}

// ==========================================================================
// Loading — multiple materials
// ==========================================================================

TEST(MaterialMultiple, LoadsAllMaterials)
{
    auto materials = Material::load_from_file("test_assets/multi.mtl");
    EXPECT_EQ(materials.size(), 3u);
}

TEST(MaterialMultiple, MaterialNamesInOrder)
{
    auto materials = Material::load_from_file("test_assets/multi.mtl");
    auto it = materials.begin();
    EXPECT_EQ(it->name(), "mat_red");
    ++it;
    EXPECT_EQ(it->name(), "mat_green");
    ++it;
    EXPECT_EQ(it->name(), "mat_blue");
}

TEST(MaterialMultiple, EachHasCorrectDiffuse)
{
    auto materials = Material::load_from_file("test_assets/multi.mtl");
    auto it = materials.begin();

    EXPECT_FLOAT_EQ(it->diffuse().r(), 1.0f);
    EXPECT_FLOAT_EQ(it->diffuse().g(), 0.0f);
    ++it;
    EXPECT_FLOAT_EQ(it->diffuse().r(), 0.0f);
    EXPECT_FLOAT_EQ(it->diffuse().g(), 1.0f);
    ++it;
    EXPECT_FLOAT_EQ(it->diffuse().r(), 0.0f);
    EXPECT_FLOAT_EQ(it->diffuse().b(), 1.0f);
}

// ==========================================================================
// Loading — full material properties
// ==========================================================================

TEST(MaterialFull, AllStandardProperties)
{
    auto materials = Material::load_from_file("test_assets/full.mtl");
    ASSERT_EQ(materials.size(), 1u);
    const auto& m = materials.front();

    EXPECT_EQ(m.name(), "full_material");
    EXPECT_FLOAT_EQ(m.ambient().r(), 0.1f);
    EXPECT_FLOAT_EQ(m.diffuse().r(), 0.8f);
    EXPECT_FLOAT_EQ(m.diffuse().g(), 0.2f);
    EXPECT_FLOAT_EQ(m.diffuse().b(), 0.3f);
    EXPECT_FLOAT_EQ(m.specular().r(), 1.0f);
    EXPECT_FLOAT_EQ(m.emissive().r(), 0.0f);
    EXPECT_FLOAT_EQ(m.shininess(), 64.0f);
    EXPECT_FLOAT_EQ(m.ior(), 1.45f);
    EXPECT_FLOAT_EQ(m.transparency(), 0.25f);
    EXPECT_EQ(m.illum(), 2);
    EXPECT_EQ(m.mapKd(), "diffuse.png");
}

TEST(MaterialFull, AllPBRProperties)
{
    auto materials = Material::load_from_file("test_assets/full.mtl");
    const auto& m = materials.front();

    EXPECT_FLOAT_EQ(m.roughness(), 0.5f);
    EXPECT_FLOAT_EQ(m.metallic(), 0.75f);
    EXPECT_FLOAT_EQ(m.sheen(), 0.1f);
    EXPECT_FLOAT_EQ(m.clearcoat(), 0.3f);
    EXPECT_FLOAT_EQ(m.clearcoatRoughness(), 0.2f);
    EXPECT_FLOAT_EQ(m.anisotropy(), 0.6f);
    EXPECT_FLOAT_EQ(m.anisotropyRotation(), 0.4f);
}

TEST(MaterialFull, TransparencyViaTr)
{
    // full.mtl uses Tr 0.25 (not d)
    auto materials = Material::load_from_file("test_assets/full.mtl");
    EXPECT_FLOAT_EQ(materials.front().transparency(), 0.25f);
}

// ==========================================================================
// Error handling
// ==========================================================================

TEST(MaterialErrors, NonExistentFileThrows)
{
    EXPECT_THROW(Material::load_from_file("test_assets/nonexistent.mtl"), std::runtime_error);
}

TEST(MaterialErrors, EmptyFileReturnsNoMaterials)
{
    // comments.obj has no newmtl keywords, so no materials
    auto materials = Material::load_from_file("test_assets/comments.obj");
    EXPECT_TRUE(materials.empty());
}
