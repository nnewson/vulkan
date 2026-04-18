#include <fire_engine/graphics/vertex.hpp>

#include <gtest/gtest.h>

using fire_engine::Vertex;

// ==========================================================================
// Construction
// ==========================================================================

TEST(VertexConstruction, DefaultIsZeroed)
{
    Vertex v;
    EXPECT_FLOAT_EQ(v.position().x(), 0.0f);
    EXPECT_FLOAT_EQ(v.position().y(), 0.0f);
    EXPECT_FLOAT_EQ(v.position().z(), 0.0f);
    EXPECT_FLOAT_EQ(v.colour().r(), 0.0f);
    EXPECT_FLOAT_EQ(v.colour().g(), 0.0f);
    EXPECT_FLOAT_EQ(v.colour().b(), 0.0f);
    EXPECT_FLOAT_EQ(v.normal().x(), 0.0f);
    EXPECT_FLOAT_EQ(v.normal().y(), 0.0f);
    EXPECT_FLOAT_EQ(v.normal().z(), 0.0f);
    EXPECT_FLOAT_EQ(v.texU(), 0.0f);
    EXPECT_FLOAT_EQ(v.texV(), 0.0f);
    EXPECT_FLOAT_EQ(v.tangent()[0], 0.0f);
    EXPECT_FLOAT_EQ(v.tangent()[1], 0.0f);
    EXPECT_FLOAT_EQ(v.tangent()[2], 0.0f);
    EXPECT_FLOAT_EQ(v.tangent()[3], 0.0f);
}

TEST(VertexConstruction, FullConstructor)
{
    Vertex v({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, {0.0f, 1.0f, 0.0f}, 0.25f, 0.75f);
    EXPECT_FLOAT_EQ(v.position().x(), 1.0f);
    EXPECT_FLOAT_EQ(v.position().y(), 2.0f);
    EXPECT_FLOAT_EQ(v.position().z(), 3.0f);
    EXPECT_FLOAT_EQ(v.colour().r(), 0.5f);
    EXPECT_FLOAT_EQ(v.colour().g(), 0.6f);
    EXPECT_FLOAT_EQ(v.colour().b(), 0.7f);
    EXPECT_FLOAT_EQ(v.normal().x(), 0.0f);
    EXPECT_FLOAT_EQ(v.normal().y(), 1.0f);
    EXPECT_FLOAT_EQ(v.normal().z(), 0.0f);
    EXPECT_FLOAT_EQ(v.texU(), 0.25f);
    EXPECT_FLOAT_EQ(v.texV(), 0.75f);
}

// ==========================================================================
// Accessors
// ==========================================================================

TEST(VertexAccessors, SetPosition)
{
    Vertex v;
    v.position({4.0f, 5.0f, 6.0f});
    EXPECT_FLOAT_EQ(v.position().x(), 4.0f);
    EXPECT_FLOAT_EQ(v.position().y(), 5.0f);
    EXPECT_FLOAT_EQ(v.position().z(), 6.0f);
}

TEST(VertexAccessors, SetColour)
{
    Vertex v;
    v.colour({0.1f, 0.2f, 0.3f});
    EXPECT_FLOAT_EQ(v.colour().r(), 0.1f);
    EXPECT_FLOAT_EQ(v.colour().g(), 0.2f);
    EXPECT_FLOAT_EQ(v.colour().b(), 0.3f);
}

TEST(VertexAccessors, SetNormal)
{
    Vertex v;
    v.normal({0.0f, 0.0f, 1.0f});
    EXPECT_FLOAT_EQ(v.normal().x(), 0.0f);
    EXPECT_FLOAT_EQ(v.normal().y(), 0.0f);
    EXPECT_FLOAT_EQ(v.normal().z(), 1.0f);
}

TEST(VertexAccessors, SetTexCoords)
{
    Vertex v;
    v.texU(0.5f);
    v.texV(0.75f);
    EXPECT_FLOAT_EQ(v.texU(), 0.5f);
    EXPECT_FLOAT_EQ(v.texV(), 0.75f);
}

TEST(VertexAccessors, SetTangent)
{
    Vertex v;
    v.tangent(0.6f, 0.7f, 0.8f, -1.0f);
    EXPECT_FLOAT_EQ(v.tangent()[0], 0.6f);
    EXPECT_FLOAT_EQ(v.tangent()[1], 0.7f);
    EXPECT_FLOAT_EQ(v.tangent()[2], 0.8f);
    EXPECT_FLOAT_EQ(v.tangent()[3], -1.0f);
}

TEST(VertexConstruction, FullConstructorWithTangent)
{
    Vertex v({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, {0.0f, 1.0f, 0.0f}, 0.25f, 0.75f,
             0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.2f, 0.3f, 1.0f);
    EXPECT_FLOAT_EQ(v.tangent()[0], 0.1f);
    EXPECT_FLOAT_EQ(v.tangent()[1], 0.2f);
    EXPECT_FLOAT_EQ(v.tangent()[2], 0.3f);
    EXPECT_FLOAT_EQ(v.tangent()[3], 1.0f);
}

TEST(VertexConstruction, TangentHandednessNegative)
{
    Vertex v({0, 0, 0}, {0, 0, 0}, {0, 1, 0}, 0, 0,
             0, 0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f);
    EXPECT_FLOAT_EQ(v.tangent()[3], -1.0f);
}

// ==========================================================================
// Equality
// ==========================================================================

TEST(VertexEquality, IdenticalVertices)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.1f, 0.2f);
    Vertex b({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.1f, 0.2f);
    EXPECT_TRUE(a == b);
}

TEST(VertexEquality, DifferentPosition)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 0.0f);
    Vertex b({9.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 0.0f);
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DifferentColour)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 0.0f);
    Vertex b({1.0f, 2.0f, 3.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.0f, 0.0f);
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DifferentNormal)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 0.0f);
    Vertex b({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 0.0f, 0.0f);
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DifferentTexU)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 0.0f);
    Vertex b({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 1.0f, 0.0f);
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DifferentTexV)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 0.0f);
    Vertex b({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 1.0f);
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DefaultVerticesAreEqual)
{
    Vertex a;
    Vertex b;
    EXPECT_TRUE(a == b);
}

// ==========================================================================
// Copy and Move Semantics
// ==========================================================================

TEST(VertexCopy, CopyConstructCreatesIndependentCopy)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.1f, 0.2f);
    Vertex b{a};
    EXPECT_TRUE(a == b);

    b.position({9.0f, 9.0f, 9.0f});
    EXPECT_FLOAT_EQ(a.position().x(), 1.0f);
}

TEST(VertexCopy, CopyAssignCreatesIndependentCopy)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.1f, 0.2f);
    Vertex b;
    b = a;
    EXPECT_TRUE(a == b);
}

TEST(VertexMove, MoveConstructTransfersState)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, 0.1f, 0.2f);
    Vertex b{std::move(a)};
    EXPECT_FLOAT_EQ(b.position().x(), 1.0f);
    EXPECT_FLOAT_EQ(b.texU(), 0.1f);
}

// ==========================================================================
// Noexcept guarantees
// ==========================================================================

TEST(VertexNoexcept, AllAccessorsAreNoexcept)
{
    Vertex v;

    static_assert(noexcept(Vertex{}));
    static_assert(noexcept(Vertex({}, {}, {}, 0.0f, 0.0f)));
    static_assert(noexcept(v.position()));
    static_assert(noexcept(v.colour()));
    static_assert(noexcept(v.normal()));
    static_assert(noexcept(v.texU()));
    static_assert(noexcept(v.texV()));
    static_assert(noexcept(v.position({})));
    static_assert(noexcept(v.colour({})));
    static_assert(noexcept(v.normal({})));
    static_assert(noexcept(v.texU(0.0f)));
    static_assert(noexcept(v.texV(0.0f)));
    static_assert(noexcept(v.tangent()));
    static_assert(noexcept(v.tangent(0.0f, 0.0f, 0.0f, 0.0f)));
    static_assert(noexcept(v == v));
}
