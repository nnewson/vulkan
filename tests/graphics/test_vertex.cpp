#include <fire_engine/graphics/vertex.hpp>

#include <gtest/gtest.h>

using fire_engine::Colour3;
using fire_engine::Joints4;
using fire_engine::Vec2;
using fire_engine::Vec3;
using fire_engine::Vec4;
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
    EXPECT_FLOAT_EQ(v.texCoord().s(), 0.0f);
    EXPECT_FLOAT_EQ(v.texCoord().t(), 0.0f);
    EXPECT_FLOAT_EQ(v.tangent().x(), 0.0f);
    EXPECT_FLOAT_EQ(v.tangent().y(), 0.0f);
    EXPECT_FLOAT_EQ(v.tangent().z(), 0.0f);
    EXPECT_FLOAT_EQ(v.tangent().w(), 0.0f);
    EXPECT_FLOAT_EQ(v.weights().x(), 0.0f);
    EXPECT_FLOAT_EQ(v.weights().y(), 0.0f);
    EXPECT_FLOAT_EQ(v.weights().z(), 0.0f);
    EXPECT_FLOAT_EQ(v.weights().w(), 0.0f);
    EXPECT_EQ(v.joints().j0(), 0u);
    EXPECT_EQ(v.joints().j1(), 0u);
    EXPECT_EQ(v.joints().j2(), 0u);
    EXPECT_EQ(v.joints().j3(), 0u);
}

TEST(VertexConstruction, FullConstructor)
{
    Vertex v({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, {0.0f, 1.0f, 0.0f}, {0.25f, 0.75f});
    EXPECT_FLOAT_EQ(v.position().x(), 1.0f);
    EXPECT_FLOAT_EQ(v.position().y(), 2.0f);
    EXPECT_FLOAT_EQ(v.position().z(), 3.0f);
    EXPECT_FLOAT_EQ(v.colour().r(), 0.5f);
    EXPECT_FLOAT_EQ(v.colour().g(), 0.6f);
    EXPECT_FLOAT_EQ(v.colour().b(), 0.7f);
    EXPECT_FLOAT_EQ(v.normal().x(), 0.0f);
    EXPECT_FLOAT_EQ(v.normal().y(), 1.0f);
    EXPECT_FLOAT_EQ(v.normal().z(), 0.0f);
    EXPECT_FLOAT_EQ(v.texCoord().s(), 0.25f);
    EXPECT_FLOAT_EQ(v.texCoord().t(), 0.75f);
}

TEST(VertexConstruction, FullConstructorWithTangent)
{
    Vertex v({1.0f, 2.0f, 3.0f}, {0.5f, 0.6f, 0.7f}, {0.0f, 1.0f, 0.0f}, {0.25f, 0.75f}, {},
             {0.0f, 0.0f, 0.0f, 0.0f}, {0.1f, 0.2f, 0.3f, 1.0f});
    EXPECT_FLOAT_EQ(v.tangent().x(), 0.1f);
    EXPECT_FLOAT_EQ(v.tangent().y(), 0.2f);
    EXPECT_FLOAT_EQ(v.tangent().z(), 0.3f);
    EXPECT_FLOAT_EQ(v.tangent().w(), 1.0f);
}

TEST(VertexConstruction, TangentHandednessNegative)
{
    Vertex v({0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0}, {}, {0.0f, 0.0f, 0.0f, 0.0f},
             {1.0f, 0.0f, 0.0f, -1.0f});
    EXPECT_FLOAT_EQ(v.tangent().w(), -1.0f);
}

TEST(VertexConstruction, WeightsConstructor)
{
    Vertex v({0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 2, 3, 4}, {0.4f, 0.3f, 0.2f, 0.1f});
    EXPECT_FLOAT_EQ(v.weights().x(), 0.4f);
    EXPECT_FLOAT_EQ(v.weights().y(), 0.3f);
    EXPECT_FLOAT_EQ(v.weights().z(), 0.2f);
    EXPECT_FLOAT_EQ(v.weights().w(), 0.1f);
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
    v.texCoord({0.5f, 0.75f});
    EXPECT_FLOAT_EQ(v.texCoord().s(), 0.5f);
    EXPECT_FLOAT_EQ(v.texCoord().t(), 0.75f);
}

TEST(VertexAccessors, SetTangent)
{
    Vertex v;
    v.tangent({0.6f, 0.7f, 0.8f, -1.0f});
    EXPECT_FLOAT_EQ(v.tangent().x(), 0.6f);
    EXPECT_FLOAT_EQ(v.tangent().y(), 0.7f);
    EXPECT_FLOAT_EQ(v.tangent().z(), 0.8f);
    EXPECT_FLOAT_EQ(v.tangent().w(), -1.0f);
}

TEST(VertexAccessors, SetJoints)
{
    Vertex v;
    v.joints({10, 20, 30, 40});
    EXPECT_EQ(v.joints().j0(), 10u);
    EXPECT_EQ(v.joints().j1(), 20u);
    EXPECT_EQ(v.joints().j2(), 30u);
    EXPECT_EQ(v.joints().j3(), 40u);
}

TEST(VertexAccessors, SetWeights)
{
    Vertex v;
    v.weights({0.25f, 0.25f, 0.25f, 0.25f});
    EXPECT_FLOAT_EQ(v.weights().x(), 0.25f);
    EXPECT_FLOAT_EQ(v.weights().y(), 0.25f);
    EXPECT_FLOAT_EQ(v.weights().z(), 0.25f);
    EXPECT_FLOAT_EQ(v.weights().w(), 0.25f);
}

// ==========================================================================
// Equality
// ==========================================================================

TEST(VertexEquality, IdenticalVertices)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.2f});
    Vertex b({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.2f});
    EXPECT_TRUE(a == b);
}

TEST(VertexEquality, DifferentPosition)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f});
    Vertex b({9.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f});
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DifferentColour)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f});
    Vertex b({1.0f, 2.0f, 3.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f});
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DifferentNormal)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f});
    Vertex b({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f});
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DifferentTexS)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f});
    Vertex b({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f});
    EXPECT_FALSE(a == b);
}

TEST(VertexEquality, DifferentTexT)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f});
    Vertex b({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f});
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
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.2f});
    Vertex b{a};
    EXPECT_TRUE(a == b);

    b.position({9.0f, 9.0f, 9.0f});
    EXPECT_FLOAT_EQ(a.position().x(), 1.0f);
}

TEST(VertexCopy, CopyAssignCreatesIndependentCopy)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.2f});
    Vertex b;
    b = a;
    EXPECT_TRUE(a == b);
}

TEST(VertexMove, MoveConstructTransfersState)
{
    Vertex a({1.0f, 2.0f, 3.0f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.1f, 0.2f});
    Vertex b{std::move(a)};
    EXPECT_FLOAT_EQ(b.position().x(), 1.0f);
    EXPECT_FLOAT_EQ(b.texCoord().s(), 0.1f);
}

// ==========================================================================
// Noexcept guarantees
// ==========================================================================

TEST(VertexNoexcept, AllAccessorsAreNoexcept)
{
    Vertex v;

    static_assert(noexcept(Vertex{}));
    static_assert(noexcept(Vertex({}, {}, {}, {})));
    static_assert(noexcept(v.position()));
    static_assert(noexcept(v.colour()));
    static_assert(noexcept(v.normal()));
    static_assert(noexcept(v.texCoord()));
    static_assert(noexcept(v.joints()));
    static_assert(noexcept(v.tangent()));
    static_assert(noexcept(v.weights()));
    static_assert(noexcept(v.position({})));
    static_assert(noexcept(v.colour({})));
    static_assert(noexcept(v.normal({})));
    static_assert(noexcept(v.texCoord({})));
    static_assert(noexcept(v.joints({})));
    static_assert(noexcept(v.tangent({})));
    static_assert(noexcept(v.weights({})));
    static_assert(noexcept(v == v));
}

// ==========================================================================
// Layout — Vulkan vertex input compatibility
// ==========================================================================

TEST(VertexLayout, SizeAndOffsets)
{
    // Vec2 and Vec4 are standard-layout with float data_[N]
    static_assert(sizeof(Vec2) == sizeof(float) * 2);
    static_assert(sizeof(Vec4) == sizeof(float) * 4);

    // Vertex size: Vec3(12) + Colour3(12) + Vec3(12) + Vec2(8) + Joints4(16)
    //            + Vec4(16) + Vec4(16) = 92 bytes
    static_assert(sizeof(Joints4) == sizeof(uint32_t) * 4);
    static_assert(sizeof(Vertex) == sizeof(Vec3) * 2 + sizeof(Colour3) + sizeof(Vec2) +
                                        sizeof(Joints4) + sizeof(Vec4) * 2);
}
