#include <fire_engine/fire_engine.hpp>
#include <gtest/gtest.h>

using namespace fire_engine;

TEST(Mat4Identity, DiagonalOnes)
{
    Mat4 m = mat4Identity();
    EXPECT_FLOAT_EQ(m.m[0], 1.0f);
    EXPECT_FLOAT_EQ(m.m[5], 1.0f);
    EXPECT_FLOAT_EQ(m.m[10], 1.0f);
    EXPECT_FLOAT_EQ(m.m[15], 1.0f);
}

TEST(Mat4Identity, OffDiagonalZeros)
{
    Mat4 m = mat4Identity();
    for (int i = 0; i < 16; ++i)
    {
        if (i == 0 || i == 5 || i == 10 || i == 15)
            continue;
        EXPECT_FLOAT_EQ(m.m[i], 0.0f) << "index " << i;
    }
}
