#include <gtest/gtest.h>

#include <algorithm>

#include <fire_engine/render/pipeline.hpp>
#include <fire_engine/render/ubo.hpp>

using fire_engine::Pipeline;

TEST(PipelineConfig, ForwardConfigIncludesIblBindings)
{
    auto config = Pipeline::forwardConfig({});

    EXPECT_EQ(config.bindings.size(), 20u);

    auto hasBinding = [&](uint32_t binding)
    {
        return std::any_of(config.bindings.begin(), config.bindings.end(),
                           [&](const auto& entry) { return entry.binding == binding; });
    };

    EXPECT_TRUE(hasBinding(12));
    EXPECT_TRUE(hasBinding(13));
    EXPECT_TRUE(hasBinding(14));
    // PCSS: non-comparison sampler over the shadow image for blocker search.
    EXPECT_TRUE(hasBinding(15));
    // KHR_materials_transmission texture.
    EXPECT_TRUE(hasBinding(16));
    // KHR_materials_clearcoat: factor (R), roughness (G), normal (RGB).
    EXPECT_TRUE(hasBinding(17));
    EXPECT_TRUE(hasBinding(18));
    EXPECT_TRUE(hasBinding(19));
}

TEST(PipelineConfig, SkyboxConfigIncludesCubemapSamplerBinding)
{
    auto config = Pipeline::skyboxConfig({});

    ASSERT_EQ(config.bindings.size(), 3u);
    EXPECT_EQ(config.bindings[0].binding, 0u);
    EXPECT_EQ(config.bindings[1].binding, 1u);
    EXPECT_EQ(config.bindings[1].descriptorType, vk::DescriptorType::eCombinedImageSampler);
    EXPECT_EQ(config.bindings[2].binding, 2u);
    EXPECT_EQ(config.bindings[2].descriptorType, vk::DescriptorType::eUniformBuffer);
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

TEST(PipelineConfig, IrradianceConvolutionConfigIncludesCubemapSamplerBinding)
{
    auto config = Pipeline::irradianceConvolutionConfig({});

    ASSERT_EQ(config.bindings.size(), 1u);
    EXPECT_EQ(config.vertShaderPath, "skybox.vert.spv");
    EXPECT_EQ(config.fragShaderPath, "irradiance_convolution.frag.spv");
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

TEST(PipelineConfig, PrefilterEnvironmentConfigIncludesCubemapSamplerBinding)
{
    auto config = Pipeline::prefilterEnvironmentConfig({});

    ASSERT_EQ(config.bindings.size(), 1u);
    EXPECT_EQ(config.vertShaderPath, "skybox.vert.spv");
    EXPECT_EQ(config.fragShaderPath, "prefilter_environment.frag.spv");
    EXPECT_FALSE(config.useVertexInput);
    EXPECT_FALSE(config.depthTestEnable);
    EXPECT_FALSE(config.depthWrite);
    EXPECT_EQ(config.bindings[0].binding, 0u);
    EXPECT_EQ(config.bindings[0].descriptorType, vk::DescriptorType::eCombinedImageSampler);
    ASSERT_EQ(config.pushConstantRanges.size(), 1u);
    EXPECT_EQ(config.pushConstantRanges[0].stageFlags, vk::ShaderStageFlagBits::eFragment);
    EXPECT_EQ(config.pushConstantRanges[0].offset, 0u);
    EXPECT_EQ(config.pushConstantRanges[0].size,
              static_cast<uint32_t>(sizeof(fire_engine::EnvironmentPrefilterPushConstants)));
}

TEST(PipelineConfig, BrdfIntegrationConfigUsesNoBindings)
{
    auto config = Pipeline::brdfIntegrationConfig({});

    EXPECT_TRUE(config.bindings.empty());
    EXPECT_EQ(config.vertShaderPath, "postprocess.vert.spv");
    EXPECT_EQ(config.fragShaderPath, "brdf_integration.frag.spv");
    EXPECT_FALSE(config.useVertexInput);
    EXPECT_FALSE(config.depthTestEnable);
    EXPECT_FALSE(config.depthWrite);
    EXPECT_TRUE(config.pushConstantRanges.empty());
}
