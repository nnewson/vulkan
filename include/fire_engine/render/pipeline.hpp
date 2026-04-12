#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/render/device.hpp>
#include <fire_engine/render/swapchain.hpp>

namespace fire_engine
{

class Pipeline
{
public:
    explicit Pipeline(const Device& device, const Swapchain& swapchain);
    ~Pipeline() = default;

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) noexcept = default;
    Pipeline& operator=(Pipeline&&) noexcept = default;

    [[nodiscard]] vk::RenderPass renderPass() const noexcept
    {
        return *renderPass_;
    }
    [[nodiscard]] vk::DescriptorSetLayout descriptorSetLayout() const noexcept
    {
        return *descSetLayout_;
    }
    [[nodiscard]] vk::PipelineLayout pipelineLayout() const noexcept
    {
        return *pipelineLayout_;
    }
    [[nodiscard]] vk::Pipeline pipeline() const noexcept
    {
        return *pipeline_;
    }

private:
    void createRenderPass(const Swapchain& swapchain);
    void createDescriptorSetLayout();
    void createGraphicsPipeline(const Swapchain& swapchain);

    [[nodiscard]] std::array<vk::PipelineShaderStageCreateInfo, 2>
    createShaderStages(vk::raii::ShaderModule& vertMod, vk::raii::ShaderModule& fragMod) const;
    void createPipelineLayout();

    const vk::raii::Device* device_{nullptr};
    vk::raii::RenderPass renderPass_{nullptr};
    vk::raii::DescriptorSetLayout descSetLayout_{nullptr};
    vk::raii::PipelineLayout pipelineLayout_{nullptr};
    vk::raii::Pipeline pipeline_{nullptr};
};

} // namespace fire_engine
