#include <fire_engine/core/shader_loader.hpp>

#include <stdexcept>

#include <gtest/gtest.h>

using fire_engine::ShaderLoader;

// ==========================================================================
// load_from_file
// ==========================================================================

TEST(ShaderLoaderLoadFromFile, LoadsFileData)
{
    auto data = ShaderLoader::load_from_file("test_assets/test_data.bin");
    EXPECT_FALSE(data.empty());
}

TEST(ShaderLoaderLoadFromFile, DataMatchesFileContents)
{
    auto data = ShaderLoader::load_from_file("test_assets/test_data.bin");
    std::string content(data.begin(), data.end());
    EXPECT_NE(content.find("HELLO"), std::string::npos);
}

TEST(ShaderLoaderLoadFromFile, SizeMatchesFileSize)
{
    auto data = ShaderLoader::load_from_file("test_assets/test_data.bin");
    EXPECT_GT(data.size(), 0u);
}

TEST(ShaderLoaderLoadFromFile, NonExistentFileThrows)
{
    EXPECT_THROW(static_cast<void>(ShaderLoader::load_from_file("test_assets/nonexistent.spv")),
                 std::runtime_error);
}

TEST(ShaderLoaderLoadFromFile, TwoLoadsReturnSameData)
{
    auto a = ShaderLoader::load_from_file("test_assets/test_data.bin");
    auto b = ShaderLoader::load_from_file("test_assets/test_data.bin");
    EXPECT_EQ(a, b);
}
