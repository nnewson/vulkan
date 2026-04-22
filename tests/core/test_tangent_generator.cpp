#include <fire_engine/core/tangent_generator.hpp>

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

using fire_engine::TangentGenerator;
using fire_engine::Vec2;
using fire_engine::Vec3;
using fire_engine::Vec4;

TEST(TangentGenerator, GeneratesTangentsForPlanarQuad)
{
    std::vector<Vec3> positions = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    std::vector<Vec3> normals(4, {0.0f, 0.0f, 1.0f});
    std::vector<Vec2> texcoords = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
    };
    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

    auto result = TangentGenerator::generate(positions, normals, texcoords, indices);

    ASSERT_TRUE(result.succeeded);
    ASSERT_EQ(result.tangents.size(), positions.size());
    for (const auto& tangent : result.tangents)
    {
        EXPECT_NEAR(tangent.x(), 1.0f, 1e-4f);
        EXPECT_NEAR(tangent.y(), 0.0f, 1e-4f);
        EXPECT_NEAR(tangent.z(), 0.0f, 1e-4f);
        EXPECT_TRUE(std::abs(tangent.w()) == 1.0f);
    }
}

TEST(TangentGenerator, GeneratedTangentsAreOrthogonalToNormals)
{
    std::vector<Vec3> positions = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    std::vector<Vec3> normals(3, {0.0f, 0.0f, 1.0f});
    std::vector<Vec2> texcoords = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
    };
    std::vector<uint32_t> indices = {0, 1, 2};

    auto result = TangentGenerator::generate(positions, normals, texcoords, indices);

    ASSERT_TRUE(result.succeeded);
    for (std::size_t i = 0; i < normals.size(); ++i)
    {
        Vec3 tangent{result.tangents[i].x(), result.tangents[i].y(), result.tangents[i].z()};
        EXPECT_NEAR(Vec3::dotProduct(normals[i], tangent), 0.0f, 1e-4f);
    }
}

TEST(TangentGenerator, GeneratesNegativeHandednessForMirroredUvs)
{
    std::vector<Vec3> positions = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    std::vector<Vec3> normals(4, {0.0f, 0.0f, 1.0f});
    std::vector<Vec2> texcoords = {
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f},
    };
    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

    auto result = TangentGenerator::generate(positions, normals, texcoords, indices);

    ASSERT_TRUE(result.succeeded);
    for (const auto& tangent : result.tangents)
    {
        EXPECT_FLOAT_EQ(tangent.w(), -1.0f);
    }
}

TEST(TangentGenerator, FailsWhenUvsAreMissing)
{
    std::vector<Vec3> positions = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    std::vector<Vec3> normals(3, {0.0f, 0.0f, 1.0f});
    std::vector<Vec2> texcoords;
    std::vector<uint32_t> indices = {0, 1, 2};

    auto result = TangentGenerator::generate(positions, normals, texcoords, indices);

    EXPECT_FALSE(result.succeeded);
    EXPECT_EQ(result.reason, "missing UVs");
}

TEST(TangentGenerator, FailsOnDegenerateUvMapping)
{
    std::vector<Vec3> positions = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    std::vector<Vec3> normals(3, {0.0f, 0.0f, 1.0f});
    std::vector<Vec2> texcoords = {
        {0.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 0.0f},
    };
    std::vector<uint32_t> indices = {0, 1, 2};

    auto result = TangentGenerator::generate(positions, normals, texcoords, indices);

    EXPECT_FALSE(result.succeeded);
    EXPECT_EQ(result.reason, "degenerate UV mapping");
}

TEST(TangentGenerator, FallsBackForVerticesOnlyInDegenerateUvTriangles)
{
    std::vector<Vec3> positions = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {2.0f, 0.0f, 0.0f},
        {2.0f, 1.0f, 0.0f},
    };
    std::vector<Vec3> normals(6, {0.0f, 0.0f, 1.0f});
    std::vector<Vec2> texcoords = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {2.0f, 0.0f},
        {2.0f, 0.0f},
    };
    std::vector<uint32_t> indices = {
        0, 1, 2,
        0, 2, 3,
        1, 4, 5,
    };

    auto result = TangentGenerator::generate(positions, normals, texcoords, indices);

    ASSERT_TRUE(result.succeeded);
    ASSERT_EQ(result.tangents.size(), positions.size());
    for (const auto& tangent : result.tangents)
    {
        Vec3 tangentDir{tangent.x(), tangent.y(), tangent.z()};
        EXPECT_GT(tangentDir.magnitudeSquared(), 0.0f);
        EXPECT_NEAR(Vec3::dotProduct(Vec3{0.0f, 0.0f, 1.0f}, tangentDir), 0.0f, 1e-4f);
        EXPECT_TRUE(std::abs(tangent.w()) == 1.0f);
    }
}
