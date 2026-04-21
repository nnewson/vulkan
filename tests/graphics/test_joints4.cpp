#include <fire_engine/graphics/joints4.hpp>
#include <gtest/gtest.h>

using fire_engine::Joints4;

// Construction

TEST(Joints4Construction, DefaultConstruction)
{
    constexpr Joints4 j;
    EXPECT_EQ(j.j0(), 0u);
    EXPECT_EQ(j.j1(), 0u);
    EXPECT_EQ(j.j2(), 0u);
    EXPECT_EQ(j.j3(), 0u);
}

TEST(Joints4Construction, ParameterisedConstruction)
{
    constexpr Joints4 j{1, 2, 3, 4};
    EXPECT_EQ(j.j0(), 1u);
    EXPECT_EQ(j.j1(), 2u);
    EXPECT_EQ(j.j2(), 3u);
    EXPECT_EQ(j.j3(), 4u);
}

TEST(Joints4Construction, PartialConstruction)
{
    constexpr Joints4 j{10, 20};
    EXPECT_EQ(j.j0(), 10u);
    EXPECT_EQ(j.j1(), 20u);
    EXPECT_EQ(j.j2(), 0u);
    EXPECT_EQ(j.j3(), 0u);
}

// Accessors

TEST(Joints4Accessors, SetJ0)
{
    Joints4 j;
    j.j0(42);
    EXPECT_EQ(j.j0(), 42u);
}

TEST(Joints4Accessors, SetJ1)
{
    Joints4 j;
    j.j1(42);
    EXPECT_EQ(j.j1(), 42u);
}

TEST(Joints4Accessors, SetJ2)
{
    Joints4 j;
    j.j2(42);
    EXPECT_EQ(j.j2(), 42u);
}

TEST(Joints4Accessors, SetJ3)
{
    Joints4 j;
    j.j3(42);
    EXPECT_EQ(j.j3(), 42u);
}

// Equality

TEST(Joints4Equality, Equal)
{
    constexpr Joints4 a{1, 2, 3, 4};
    constexpr Joints4 b{1, 2, 3, 4};
    EXPECT_EQ(a, b);
}

TEST(Joints4Equality, NotEqual)
{
    constexpr Joints4 a{1, 2, 3, 4};
    constexpr Joints4 b{1, 2, 3, 5};
    EXPECT_NE(a, b);
}

TEST(Joints4Equality, DefaultsEqual)
{
    constexpr Joints4 a;
    constexpr Joints4 b;
    EXPECT_EQ(a, b);
}

// Copy and Move

TEST(Joints4Copy, CopyConstruction)
{
    constexpr Joints4 original{10, 20, 30, 40};
    constexpr Joints4 copy(original);
    EXPECT_EQ(copy, original);
}

TEST(Joints4Copy, CopyAssignment)
{
    Joints4 a{10, 20, 30, 40};
    Joints4 b;
    b = a;
    EXPECT_EQ(b, a);
}

TEST(Joints4Move, MoveConstruction)
{
    Joints4 original{10, 20, 30, 40};
    Joints4 moved(std::move(original));
    EXPECT_EQ(moved, (Joints4{10, 20, 30, 40}));
}

TEST(Joints4Move, MoveAssignment)
{
    Joints4 original{10, 20, 30, 40};
    Joints4 target;
    target = std::move(original);
    EXPECT_EQ(target, (Joints4{10, 20, 30, 40}));
}

// Noexcept

TEST(Joints4Noexcept, ConstructionIsNoexcept)
{
    EXPECT_TRUE(noexcept(Joints4{}));
    EXPECT_TRUE(noexcept(Joints4{1, 2, 3, 4}));
}

TEST(Joints4Noexcept, MoveIsNoexcept)
{
    EXPECT_TRUE(noexcept(Joints4(std::declval<Joints4&&>())));
    Joints4 j;
    Joints4 other;
    EXPECT_TRUE(noexcept(j = std::move(other)));
}

TEST(Joints4Noexcept, GettersAreNoexcept)
{
    Joints4 j;
    EXPECT_TRUE(noexcept(j.j0()));
    EXPECT_TRUE(noexcept(j.j1()));
    EXPECT_TRUE(noexcept(j.j2()));
    EXPECT_TRUE(noexcept(j.j3()));
}

TEST(Joints4Noexcept, SettersAreNoexcept)
{
    Joints4 j;
    EXPECT_TRUE(noexcept(j.j0(0)));
    EXPECT_TRUE(noexcept(j.j1(0)));
    EXPECT_TRUE(noexcept(j.j2(0)));
    EXPECT_TRUE(noexcept(j.j3(0)));
}

// Constexpr

TEST(Joints4Constexpr, ConstexprConstruction)
{
    constexpr Joints4 j{5, 10, 15, 20};
    static_assert(j.j0() == 5);
    static_assert(j.j1() == 10);
    static_assert(j.j2() == 15);
    static_assert(j.j3() == 20);
}

TEST(Joints4Constexpr, ConstexprEquality)
{
    constexpr Joints4 a{1, 2, 3, 4};
    constexpr Joints4 b{1, 2, 3, 4};
    static_assert(a == b);
}

// Layout

TEST(Joints4Layout, SizeMatchesRawArray)
{
    static_assert(sizeof(Joints4) == sizeof(uint32_t) * 4);
}
