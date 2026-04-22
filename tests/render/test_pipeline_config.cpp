#include <gtest/gtest.h>

#include <algorithm>

#include <fire_engine/render/pipeline.hpp>
#include <fire_engine/render/ubo.hpp>

using fire_engine::Pipeline;

TEST(PipelineConfig, ForwardConfigIncludesIblBindings)
{
    auto config = Pipeline::forwardConfig({});

    EXPECT_EQ(config.bindings.size(), 15u);

    auto hasBinding = [&](uint32_t binding)
    {
        return std::any_of(config.bindings.begin(), config.bindings.end(),
                           [&](const auto& entry) { return entry.binding == binding; });
    };

    EXPECT_TRUE(hasBinding(12));
    EXPECT_TRUE(hasBinding(13));
    EXPECT_TRUE(hasBinding(14));
}

TEST(PipelineConfig, SkyboxConfigIncludesCubemapSamplerBinding)
{
    auto config = Pipeline::skyboxConfig({});

    ASSERT_EQ(config.bindings.size(), 2u);
    EXPECT_EQ(config.bindings[0].binding, 0u);
    EXPECT_EQ(config.bindings[1].binding, 1u);
    EXPECT_EQ(config.bindings[1].descriptorType, vk::DescriptorType::eCombinedImageSampler);
}

TEST(PipelineConfig, EnvironmentConvertConfigIncludesPanoramaSamplerBinding)
{
    auto config = Pipeline::environmentConvertConfig({});

    ASSERT_EQ(config.bindings.size(), 1u);
    EXPECT_EQ(config.vertShaderPath, "skybox.vert.spv");
    EXPECT_EQ(config.fragShaderPath, "environment_convert.frag.spv");
    EXPECT_FALSE(config.useVertexInput);
    EXPECT_FALSE(config.depthTestEnable);
    EXPECT_FALSE(config.depthWrite);
    EXPECT_EQ(config.bindings[0].binding, 0u);
    EXPECT_EQ(config.bindings[0].descriptorType, vk::DescriptorType::eCombinedImageSampler);
    ASSERT_EQ(config.pushConstantRanges.size(), 1u);
    EXPECT_EQ(config.pushConstantRanges[0].stageFlags, vk::ShaderStageFlagBits::eFragment);
    EXPECT_EQ(config.pushConstantRanges[0].offset, 0u);
    EXPECT_EQ(config.pushConstantRanges[0].size,
              static_cast<uint32_t>(sizeof(fire_engine::EnvironmentCaptureUBO)));
}
