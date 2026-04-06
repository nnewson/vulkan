#include <fire_engine/graphics/geometry.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

#include <gtest/gtest.h>

using fire_engine::Colour3;
using fire_engine::Geometry;
using fire_engine::Material;

// ==========================================================================
// Loading — basic triangle
// ==========================================================================

TEST(GeometryLoading, LoadsTriangle)
{
    Geometry geo = Geometry::load_from_file("test_assets/triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    EXPECT_EQ(data.vertices.size(), 3u);
    EXPECT_EQ(data.indices.size(), 3u);
}

TEST(GeometryLoading, TrianglePositions)
{
    Geometry geo = Geometry::load_from_file("test_assets/triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    // Vertices should match the OBJ positions
    EXPECT_FLOAT_EQ(data.vertices[0].position().x(), 0.0f);
    EXPECT_FLOAT_EQ(data.vertices[0].position().y(), 0.0f);
    EXPECT_FLOAT_EQ(data.vertices[0].position().z(), 0.0f);

    EXPECT_FLOAT_EQ(data.vertices[1].position().x(), 1.0f);
    EXPECT_FLOAT_EQ(data.vertices[1].position().y(), 0.0f);

    EXPECT_FLOAT_EQ(data.vertices[2].position().x(), 0.0f);
    EXPECT_FLOAT_EQ(data.vertices[2].position().y(), 1.0f);
}

TEST(GeometryLoading, DefaultColourIsWhite)
{
    Geometry geo = Geometry::load_from_file("test_assets/triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    for (const auto& v : data.vertices)
    {
        EXPECT_FLOAT_EQ(v.colour().r(), 1.0f);
        EXPECT_FLOAT_EQ(v.colour().g(), 1.0f);
        EXPECT_FLOAT_EQ(v.colour().b(), 1.0f);
    }
}

// ==========================================================================
// Loading — computed normals
// ==========================================================================

TEST(GeometryNormals, ComputesNormalsWhenMissing)
{
    Geometry geo = Geometry::load_from_file("test_assets/triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    // Triangle in XY plane, normal should point in +Z or -Z direction
    for (const auto& v : data.vertices)
    {
        EXPECT_FLOAT_EQ(v.normal().x(), 0.0f);
        EXPECT_FLOAT_EQ(v.normal().y(), 0.0f);
        EXPECT_NE(v.normal().z(), 0.0f);
    }
}

TEST(GeometryNormals, UsesExplicitNormals)
{
    Geometry geo = Geometry::load_from_file("test_assets/textured_triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    for (const auto& v : data.vertices)
    {
        EXPECT_FLOAT_EQ(v.normal().x(), 0.0f);
        EXPECT_FLOAT_EQ(v.normal().y(), 0.0f);
        EXPECT_FLOAT_EQ(v.normal().z(), 1.0f);
    }
}

// ==========================================================================
// Loading — texture coordinates
// ==========================================================================

TEST(GeometryTexCoords, LoadsTexCoords)
{
    Geometry geo = Geometry::load_from_file("test_assets/textured_triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    EXPECT_EQ(data.vertices.size(), 3u);

    // vt 0.0 0.0 / vt 1.0 0.0 / vt 0.0 1.0
    EXPECT_FLOAT_EQ(data.vertices[0].texU(), 0.0f);
    EXPECT_FLOAT_EQ(data.vertices[0].texV(), 0.0f);
    EXPECT_FLOAT_EQ(data.vertices[1].texU(), 1.0f);
    EXPECT_FLOAT_EQ(data.vertices[1].texV(), 0.0f);
    EXPECT_FLOAT_EQ(data.vertices[2].texU(), 0.0f);
    EXPECT_FLOAT_EQ(data.vertices[2].texV(), 1.0f);
}

TEST(GeometryTexCoords, DefaultTexCoordsAreZero)
{
    Geometry geo = Geometry::load_from_file("test_assets/triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    for (const auto& v : data.vertices)
    {
        EXPECT_FLOAT_EQ(v.texU(), 0.0f);
        EXPECT_FLOAT_EQ(v.texV(), 0.0f);
    }
}

// ==========================================================================
// Loading — face formats
// ==========================================================================

TEST(GeometryFaceFormats, VertexOnly)
{
    // f v v v
    Geometry geo = Geometry::load_from_file("test_assets/triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});
    EXPECT_EQ(data.vertices.size(), 3u);
    EXPECT_EQ(data.indices.size(), 3u);
}

TEST(GeometryFaceFormats, VertexTexcoordNormal)
{
    // f v/vt/vn
    Geometry geo = Geometry::load_from_file("test_assets/textured_triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});
    EXPECT_EQ(data.vertices.size(), 3u);
}

TEST(GeometryFaceFormats, VertexTexcoord)
{
    // f v/vt
    Geometry geo = Geometry::load_from_file("test_assets/no_normals_triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});
    EXPECT_EQ(data.vertices.size(), 3u);
}

TEST(GeometryFaceFormats, VertexSlashSlashNormal)
{
    // f v//vn
    Geometry geo = Geometry::load_from_file("test_assets/skip_normals_triangle.obj");
    auto data = geo.to_coloured_indexed_geometry({});
    EXPECT_EQ(data.vertices.size(), 3u);

    for (const auto& v : data.vertices)
    {
        EXPECT_FLOAT_EQ(v.normal().z(), 1.0f);
    }
}

// ==========================================================================
// Fan triangulation
// ==========================================================================

TEST(GeometryTriangulation, QuadProducesTwoTriangles)
{
    Geometry geo = Geometry::load_from_file("test_assets/quad.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    // 4 vertices, 2 triangles = 6 indices
    EXPECT_EQ(data.vertices.size(), 4u);
    EXPECT_EQ(data.indices.size(), 6u);
}

TEST(GeometryTriangulation, QuadSharesVerticesBetweenTriangles)
{
    Geometry geo = Geometry::load_from_file("test_assets/quad.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    // Two triangles from a quad must share exactly 2 vertices
    std::set<uint16_t> tri1{data.indices[0], data.indices[1], data.indices[2]};
    std::set<uint16_t> tri2{data.indices[3], data.indices[4], data.indices[5]};
    std::vector<uint16_t> shared;
    std::set_intersection(tri1.begin(), tri1.end(), tri2.begin(), tri2.end(),
                          std::back_inserter(shared));
    EXPECT_EQ(shared.size(), 2u);
}

// ==========================================================================
// Material application
// ==========================================================================

TEST(GeometryMaterial, AppliesMaterialColour)
{
    Geometry geo = Geometry::load_from_file("test_assets/with_material.obj");

    Material red;
    red.name("red_material");
    red.diffuse(Colour3{1.0f, 0.0f, 0.0f});

    auto data = geo.to_coloured_indexed_geometry({red});

    for (const auto& v : data.vertices)
    {
        EXPECT_FLOAT_EQ(v.colour().r(), 1.0f);
        EXPECT_FLOAT_EQ(v.colour().g(), 0.0f);
        EXPECT_FLOAT_EQ(v.colour().b(), 0.0f);
    }
}

TEST(GeometryMaterial, UnknownMaterialUsesWhite)
{
    Geometry geo = Geometry::load_from_file("test_assets/with_material.obj");

    // Pass empty materials list — material name won't match
    auto data = geo.to_coloured_indexed_geometry({});

    for (const auto& v : data.vertices)
    {
        EXPECT_FLOAT_EQ(v.colour().r(), 1.0f);
        EXPECT_FLOAT_EQ(v.colour().g(), 1.0f);
        EXPECT_FLOAT_EQ(v.colour().b(), 1.0f);
    }
}

// ==========================================================================
// Comments
// ==========================================================================

TEST(GeometryComments, InlineCommentsStripped)
{
    Geometry geo = Geometry::load_from_file("test_assets/comments.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    EXPECT_EQ(data.vertices.size(), 3u);
    EXPECT_EQ(data.indices.size(), 3u);

    // First vertex should still be (0,0,0) despite inline comment
    EXPECT_FLOAT_EQ(data.vertices[0].position().x(), 0.0f);
}

// ==========================================================================
// Error handling
// ==========================================================================

TEST(GeometryErrors, NonExistentFileThrows)
{
    EXPECT_THROW(Geometry::load_from_file("test_assets/nonexistent.obj"), std::runtime_error);
}

// ==========================================================================
// Vertex deduplication
// ==========================================================================

TEST(GeometryDedup, SharedVerticesAreDeduped)
{
    // The quad has 4 unique vertices but 6 indices (2 triangles)
    Geometry geo = Geometry::load_from_file("test_assets/quad.obj");
    auto data = geo.to_coloured_indexed_geometry({});

    EXPECT_EQ(data.vertices.size(), 4u);
    EXPECT_EQ(data.indices.size(), 6u);
}
