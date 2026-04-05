#include <fire_engine/core/model_loader.hpp>

#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

using fire_engine::ModelLoader;

// ==========================================================================
// trim_comment
// ==========================================================================

TEST(ModelLoaderTrimComment, RemovesTrailingComment)
{
    std::string line = "v 1.0 2.0 3.0 # position";
    ModelLoader::trim_comment(line);
    EXPECT_EQ(line, "v 1.0 2.0 3.0 ");
}

TEST(ModelLoaderTrimComment, NoCommentUnchanged)
{
    std::string line = "v 1.0 2.0 3.0";
    ModelLoader::trim_comment(line);
    EXPECT_EQ(line, "v 1.0 2.0 3.0");
}

TEST(ModelLoaderTrimComment, EntireLineIsComment)
{
    std::string line = "# this is a comment";
    ModelLoader::trim_comment(line);
    EXPECT_EQ(line, "");
}

TEST(ModelLoaderTrimComment, EmptyString)
{
    std::string line;
    ModelLoader::trim_comment(line);
    EXPECT_EQ(line, "");
}

TEST(ModelLoaderTrimComment, MultipleHashes)
{
    std::string line = "data # comment # more";
    ModelLoader::trim_comment(line);
    EXPECT_EQ(line, "data ");
}

// ==========================================================================
// trim
// ==========================================================================

TEST(ModelLoaderTrim, LeadingSpaces)
{
    std::string s = "   hello";
    ModelLoader::trim(s);
    EXPECT_EQ(s, "hello");
}

TEST(ModelLoaderTrim, TrailingSpaces)
{
    std::string s = "hello   ";
    ModelLoader::trim(s);
    EXPECT_EQ(s, "hello");
}

TEST(ModelLoaderTrim, BothSides)
{
    std::string s = "  hello  ";
    ModelLoader::trim(s);
    EXPECT_EQ(s, "hello");
}

TEST(ModelLoaderTrim, Tabs)
{
    std::string s = "\thello\t";
    ModelLoader::trim(s);
    EXPECT_EQ(s, "hello");
}

TEST(ModelLoaderTrim, CarriageReturn)
{
    std::string s = "hello\r\n";
    ModelLoader::trim(s);
    EXPECT_EQ(s, "hello");
}

TEST(ModelLoaderTrim, AllWhitespace)
{
    std::string s = "   \t\r\n  ";
    ModelLoader::trim(s);
    EXPECT_EQ(s, "");
}

TEST(ModelLoaderTrim, EmptyString)
{
    std::string s;
    ModelLoader::trim(s);
    EXPECT_EQ(s, "");
}

TEST(ModelLoaderTrim, NoWhitespace)
{
    std::string s = "hello";
    ModelLoader::trim(s);
    EXPECT_EQ(s, "hello");
}

TEST(ModelLoaderTrim, InternalWhitespacePreserved)
{
    std::string s = "  hello world  ";
    ModelLoader::trim(s);
    EXPECT_EQ(s, "hello world");
}

// ==========================================================================
// parse_file
// ==========================================================================

TEST(ModelLoaderParseFile, ParsesKeywordsAndValues)
{
    std::vector<std::pair<std::string, std::string>> parsed;

    ModelLoader::load_from_file("test_assets/triangle.obj",
        [&](const std::string& keyword, std::istringstream& iss)
        {
            std::string rest;
            std::getline(iss, rest);
            ModelLoader::trim(rest);
            parsed.emplace_back(keyword, rest);
        });

    // 3 vertices + 1 face = 4 lines (comment skipped)
    EXPECT_EQ(parsed.size(), 4u);
    EXPECT_EQ(parsed[0].first, "v");
    EXPECT_EQ(parsed[3].first, "f");
}

TEST(ModelLoaderParseFile, SkipsCommentsAndBlankLines)
{
    std::vector<std::string> keywords;

    ModelLoader::load_from_file("test_assets/comments.obj",
        [&](const std::string& keyword, std::istringstream&)
        {
            keywords.push_back(keyword);
        });

    // Only v, v, v, f should come through
    EXPECT_EQ(keywords.size(), 4u);
    for (const auto& kw : keywords)
    {
        EXPECT_TRUE(kw == "v" || kw == "f");
    }
}

TEST(ModelLoaderParseFile, NonExistentFileThrows)
{
    EXPECT_THROW(
        ModelLoader::load_from_file("test_assets/nonexistent.obj",
            [](const std::string&, std::istringstream&) {}),
        std::runtime_error);
}
