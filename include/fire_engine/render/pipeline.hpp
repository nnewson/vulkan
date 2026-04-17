#pragma once

#include <array>
#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/render/device.hpp>
#include <fire_engine/render/swapchain.hpp>

namespace fire_engine
{

struct PipelineConfig
{
    std::string vertShaderPath;
    std::string fragShaderPath;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    vk::RenderPass renderPass{};
};

class Pipeline
{
public:
    Pipeline(const Device& device, const Swapchain& swapchain, const PipelineConfig& config);
    ~Pipeline() = default;

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) noexcept = default;
    Pipeline& operator=(Pipeline&&) noexcept = default;

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

    // Factory helpers for the existing forward-lit pipeline — used until
    // dedicated RenderPass/pipeline types are extracted in later steps.
    [[nodiscard]] static vk::raii::RenderPass createForwardRenderPass(const Device& device,
                                                                      const Swapchain& swapchain);
    [[nodiscard]] static PipelineConfig forwardConfig(vk::RenderPass renderPass);

private:
    void createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings);
    void createGraphicsPipeline(const Swapchain& swapchain, const PipelineConfig& config);

    [[nodiscard]] std::array<vk::PipelineShaderStageCreateInfo, 2>
    createShaderStages(const PipelineConfig& config, vk::raii::ShaderModule& vertMod,
                       vk::raii::ShaderModule& fragMod) const;
    void createPipelineLayout();

    const vk::raii::Device* device_{nullptr};
    vk::raii::DescriptorSetLayout descSetLayout_{nullptr};
    vk::raii::PipelineLayout pipelineLayout_{nullptr};
    vk::raii::Pipeline pipeline_{nullptr};
};

} // namespace fire_engine
