#include <gtest/gtest.h>

#include <fire_engine/graphics/texture.hpp>

using namespace fire_engine;

// ---------------------------------------------------------------------------
// Default construction
// ---------------------------------------------------------------------------

TEST(Texture, DefaultNotLoaded)
{
    Texture tex;
    EXPECT_FALSE(tex.loaded());
}

TEST(Texture, DefaultHandleIsNull)
{
    Texture tex;
    EXPECT_EQ(tex.handle(), NullTexture);
}

// ---------------------------------------------------------------------------
// Move semantics
// ---------------------------------------------------------------------------

TEST(Texture, IsNonCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<Texture>);
    EXPECT_FALSE(std::is_copy_assignable_v<Texture>);
}

TEST(Texture, IsNothrowMovable)
{
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<Texture>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<Texture>);
}

TEST(Texture, MoveConstructionTransfersState)
{
    Texture original;
    Texture moved(std::move(original));
    EXPECT_FALSE(moved.loaded());
    EXPECT_EQ(moved.handle(), NullTexture);
}

TEST(Texture, MoveAssignmentTransfersState)
{
    Texture original;
    Texture target;
    target = std::move(original);
    EXPECT_FALSE(target.loaded());
}
