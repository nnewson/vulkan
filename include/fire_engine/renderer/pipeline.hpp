#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include <fire_engine/renderer/device.hpp>
#include <fire_engine/renderer/swapchain.hpp>

namespace fire_engine
{

class Pipeline
{
public:
    explicit Pipeline(const Device& device, const Swapchain& swapchain);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) noexcept = default;
    Pipeline& operator=(Pipeline&&) noexcept = default;

    [[nodiscard]] vk::RenderPass renderPass() const noexcept
    {
        return renderPass_;
    }
    [[nodiscard]] vk::DescriptorSetLayout descriptorSetLayout() const noexcept
    {
        return descSetLayout_;
    }
    [[nodiscard]] vk::PipelineLayout pipelineLayout() const noexcept
    {
        return pipelineLayout_;
    }
    [[nodiscard]] vk::Pipeline pipeline() const noexcept
    {
        return pipeline_;
    }

private:
    void createRenderPass(const Swapchain& swapchain);
    void createDescriptorSetLayout();
    void createGraphicsPipeline(const Swapchain& swapchain);

    [[nodiscard]] vk::ShaderModule createShaderModule(const std::vector<char>& code);

    vk::Device device_; // stored for cleanup
    vk::RenderPass renderPass_;
    vk::DescriptorSetLayout descSetLayout_;
    vk::PipelineLayout pipelineLayout_;
    vk::Pipeline pipeline_;
};

} // namespace fire_engine
