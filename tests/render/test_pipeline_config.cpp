#include <gtest/gtest.h>

#include <algorithm>

#include <fire_engine/render/pipeline.hpp>

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
