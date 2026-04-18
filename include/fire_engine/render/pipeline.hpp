#pragma once

#include <array>
#include <string>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/render/device.hpp>

namespace fire_engine
{

struct PipelineConfig
{
    std::string vertShaderPath;
    std::string fragShaderPath;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    vk::RenderPass renderPass{};
    bool useVertexInput{true};
    bool depthWrite{true};
    vk::CompareOp depthCompare{vk::CompareOp::eLess};
    vk::CullModeFlags cullMode{vk::CullModeFlagBits::eBack};
    bool blendEnable{false};
    vk::BlendFactor srcColorBlend{vk::BlendFactor::eOne};
    vk::BlendFactor dstColorBlend{vk::BlendFactor::eZero};
    vk::BlendFactor srcAlphaBlend{vk::BlendFactor::eOne};
    vk::BlendFactor dstAlphaBlend{vk::BlendFactor::eZero};
};

class Pipeline
{
public:
    Pipeline(const Device& device, const PipelineConfig& config);
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

    // Factory producing the PipelineConfig for the existing forward-lit
    // pipeline. The render pass handle comes from the caller (usually a
    // RenderPass::createForward() result).
    [[nodiscard]] static PipelineConfig forwardConfig(vk::RenderPass renderPass);

    // Variant of forwardConfig with cullMode=None for glTF materials flagged
    // doubleSided. Otherwise identical to forwardConfig (shaders, bindings,
    // depth state).
    [[nodiscard]] static PipelineConfig forwardDoubleSidedConfig(vk::RenderPass renderPass);

    // Variant of forwardConfig for glTF BLEND materials: cullMode=None,
    // depthWrite disabled, straight-alpha blending
    // (SRC_ALPHA / ONE_MINUS_SRC_ALPHA on colour; ONE / ONE_MINUS_SRC_ALPHA on
    // alpha).
    [[nodiscard]] static PipelineConfig forwardBlendConfig(vk::RenderPass renderPass);

    // Factory producing the PipelineConfig for a procedural skybox pipeline.
    // Shares the forward render pass, uses no vertex buffers (fullscreen
    // triangle via gl_VertexIndex), and disables depth writes with LEQUAL
    // compare so it only writes where no forward geometry has drawn.
    [[nodiscard]] static PipelineConfig skyboxConfig(vk::RenderPass renderPass);

private:
    void createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings);
    void createGraphicsPipeline(const PipelineConfig& config);

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
