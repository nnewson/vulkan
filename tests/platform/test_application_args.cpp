#include <fire_engine/platform/application_args.hpp>

#include <gtest/gtest.h>

using fire_engine::parseApplicationArgs;

TEST(ApplicationArgs, EmptyArgsUseDefaults)
{
    char program[] = "fireEngineApp";
    char* argv[] = {program};

    const auto args = parseApplicationArgs(1, argv);

    EXPECT_TRUE(args.scenePath.empty());
    EXPECT_TRUE(args.skyboxPath.empty());
}

TEST(ApplicationArgs, SingleSceneArgumentSetsScenePath)
{
    char program[] = "fireEngineApp";
    char scene[] = "Fox/Fox.gltf";
    char* argv[] = {program, scene};

    const auto args = parseApplicationArgs(2, argv);

    EXPECT_EQ(args.scenePath, "Fox/Fox.gltf");
    EXPECT_TRUE(args.skyboxPath.empty());
}

TEST(ApplicationArgs, SingleHdrArgumentSetsSkyboxPath)
{
    char program[] = "fireEngineApp";
    char skybox[] = "nightbox.hdr";
    char* argv[] = {program, skybox};

    const auto args = parseApplicationArgs(2, argv);

    EXPECT_TRUE(args.scenePath.empty());
    EXPECT_EQ(args.skyboxPath, "nightbox.hdr");
}

TEST(ApplicationArgs, SingleExrArgumentSetsSkyboxPath)
{
    char program[] = "fireEngineApp";
    char skybox[] = "studio.EXR";
    char* argv[] = {program, skybox};

    const auto args = parseApplicationArgs(2, argv);

    EXPECT_TRUE(args.scenePath.empty());
    EXPECT_EQ(args.skyboxPath, "studio.EXR");
}

TEST(ApplicationArgs, TwoArgumentsKeepSceneThenSkyboxOrder)
{
    char program[] = "fireEngineApp";
    char scene[] = "Fox/Fox.gltf";
    char skybox[] = "nightbox.hdr";
    char* argv[] = {program, scene, skybox};

    const auto args = parseApplicationArgs(3, argv);

    EXPECT_EQ(args.scenePath, "Fox/Fox.gltf");
    EXPECT_EQ(args.skyboxPath, "nightbox.hdr");
}
