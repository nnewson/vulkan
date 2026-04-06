#include <fire_engine/graphics/image.hpp>

#include <stdexcept>

#include <gtest/gtest.h>

using fire_engine::Image;

// ==========================================================================
// Construction
// ==========================================================================

TEST(ImageConstruction, DefaultIsEmpty)
{
    Image img;
    EXPECT_EQ(img.width(), 0);
    EXPECT_EQ(img.height(), 0);
    EXPECT_EQ(img.channels(), 0);
    EXPECT_EQ(img.size_bytes(), 0u);
    EXPECT_TRUE(img.empty());
    EXPECT_EQ(img.data(), nullptr);
}

// ==========================================================================
// Loading
// ==========================================================================

TEST(ImageLoading, LoadValidPng)
{
    Image img = Image::load_from_file("test_assets/test_2x2.png");
    EXPECT_EQ(img.width(), 2);
    EXPECT_EQ(img.height(), 2);
    EXPECT_EQ(img.channels(), 4);
    EXPECT_FALSE(img.empty());
}

TEST(ImageLoading, SizeBytesMatchesDimensions)
{
    Image img = Image::load_from_file("test_assets/test_2x2.png");
    std::size_t expected = static_cast<std::size_t>(img.width()) * img.height() * img.channels();
    EXPECT_EQ(img.size_bytes(), expected);
}

TEST(ImageLoading, AlwaysLoadsAsRGBA)
{
    Image img = Image::load_from_file("test_assets/test_2x2.png");
    EXPECT_EQ(img.channels(), 4);
}

TEST(ImageLoading, DataIsNotNull)
{
    Image img = Image::load_from_file("test_assets/test_2x2.png");
    EXPECT_NE(img.data(), nullptr);
}

TEST(ImageLoading, PixelDataIsCorrect)
{
    Image img = Image::load_from_file("test_assets/test_2x2.png");
    const uint8_t* d = img.data();

    // Row 0, Col 0: red (255, 0, 0, 255)
    EXPECT_EQ(d[0], 255);
    EXPECT_EQ(d[1], 0);
    EXPECT_EQ(d[2], 0);
    EXPECT_EQ(d[3], 255);

    // Row 0, Col 1: green (0, 255, 0, 255)
    EXPECT_EQ(d[4], 0);
    EXPECT_EQ(d[5], 255);
    EXPECT_EQ(d[6], 0);
    EXPECT_EQ(d[7], 255);

    // Row 1, Col 0: blue (0, 0, 255, 255)
    EXPECT_EQ(d[8], 0);
    EXPECT_EQ(d[9], 0);
    EXPECT_EQ(d[10], 255);
    EXPECT_EQ(d[11], 255);

    // Row 1, Col 1: white (255, 255, 255, 255)
    EXPECT_EQ(d[12], 255);
    EXPECT_EQ(d[13], 255);
    EXPECT_EQ(d[14], 255);
    EXPECT_EQ(d[15], 255);
}

TEST(ImageLoading, NonExistentFileThrows)
{
    EXPECT_THROW(Image::load_from_file("test_assets/nonexistent.png"), std::runtime_error);
}

TEST(ImageLoading, InvalidFileThrows)
{
    EXPECT_THROW(Image::load_from_file("test_assets/"), std::runtime_error);
}

// ==========================================================================
// Move Semantics
// ==========================================================================

TEST(ImageMove, MoveConstructTransfersData)
{
    Image a = Image::load_from_file("test_assets/test_2x2.png");
    const uint8_t* originalData = a.data();
    int originalWidth = a.width();

    Image b{std::move(a)};

    EXPECT_EQ(b.width(), originalWidth);
    EXPECT_EQ(b.data(), originalData);
    EXPECT_FALSE(b.empty());
}

TEST(ImageMove, MoveConstructZerosSource)
{
    Image a = Image::load_from_file("test_assets/test_2x2.png");
    Image b{std::move(a)};

    EXPECT_EQ(a.width(), 0);
    EXPECT_EQ(a.height(), 0);
    EXPECT_EQ(a.channels(), 0);
    EXPECT_TRUE(a.empty());
}

TEST(ImageMove, MoveAssignTransfersData)
{
    Image a = Image::load_from_file("test_assets/test_2x2.png");
    int originalWidth = a.width();

    Image b;
    b = std::move(a);

    EXPECT_EQ(b.width(), originalWidth);
    EXPECT_FALSE(b.empty());
    EXPECT_EQ(a.width(), 0);
    EXPECT_TRUE(a.empty());
}

TEST(ImageMove, MoveAssignSelfIsNoOp)
{
    Image a = Image::load_from_file("test_assets/test_2x2.png");
    int originalWidth = a.width();

    auto& ref = a;
    a = std::move(ref);

    EXPECT_EQ(a.width(), originalWidth);
    EXPECT_FALSE(a.empty());
}

// ==========================================================================
// Copy Semantics
// ==========================================================================

TEST(ImageCopy, CopyConstructCreatesIndependentCopy)
{
    Image a = Image::load_from_file("test_assets/test_2x2.png");
    Image b{a};

    EXPECT_EQ(b.width(), a.width());
    EXPECT_EQ(b.height(), a.height());
    EXPECT_EQ(b.channels(), a.channels());
    EXPECT_EQ(b.size_bytes(), a.size_bytes());
    EXPECT_NE(b.data(), a.data());

    // Pixel data should match
    for (std::size_t i = 0; i < a.size_bytes(); ++i)
    {
        EXPECT_EQ(b.data()[i], a.data()[i]) << "mismatch at byte " << i;
    }
}

TEST(ImageCopy, CopyAssignCreatesIndependentCopy)
{
    Image a = Image::load_from_file("test_assets/test_2x2.png");
    Image b;
    b = a;

    EXPECT_EQ(b.width(), a.width());
    EXPECT_NE(b.data(), a.data());
    EXPECT_FALSE(a.empty());
}

// ==========================================================================
// Multiple Loads
// ==========================================================================

TEST(ImageMultipleLoads, TwoLoadsAreIndependent)
{
    Image a = Image::load_from_file("test_assets/test_2x2.png");
    Image b = Image::load_from_file("test_assets/test_2x2.png");

    EXPECT_EQ(a.width(), b.width());
    EXPECT_EQ(a.size_bytes(), b.size_bytes());
    EXPECT_NE(a.data(), b.data());
}
