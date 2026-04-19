#include <cstddef>

#include <fire_engine/core/shader_loader.hpp>
#include <fire_engine/graphics/vertex.hpp>
#include <fire_engine/render/pipeline.hpp>

namespace fire_engine
{

Pipeline::Pipeline(const Device& device, const PipelineConfig& config)
    : device_(&device.device())
{
    createDescriptorSetLayout(config.bindings);
    createGraphicsPipeline(config);
}

PipelineConfig Pipeline::forwardConfig(vk::RenderPass renderPass)
{
    PipelineConfig config;
    config.vertShaderPath = "shader.vert.spv";
    config.fragShaderPath = "shader.frag.spv";
    config.bindings = {
        {0, vk::DescriptorType::eUniformBuffer, 1,
         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
        {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
        {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
        {3, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        {4, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        {5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        {6, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
        {7, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
        {8, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
        {9, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
    };
    config.renderPass = renderPass;
    return config;
}

PipelineConfig Pipeline::forwardDoubleSidedConfig(vk::RenderPass renderPass)
{
    PipelineConfig config = forwardConfig(renderPass);
    config.cullMode = vk::CullModeFlagBits::eNone;
    return config;
}

PipelineConfig Pipeline::forwardBlendConfig(vk::RenderPass renderPass)
{
    PipelineConfig config = forwardConfig(renderPass);
    config.cullMode = vk::CullModeFlagBits::eNone;
    config.depthWrite = false;
    config.blendEnable = true;
    config.srcColorBlend = vk::BlendFactor::eSrcAlpha;
    config.dstColorBlend = vk::BlendFactor::eOneMinusSrcAlpha;
    config.srcAlphaBlend = vk::BlendFactor::eOne;
    config.dstAlphaBlend = vk::BlendFactor::eOneMinusSrcAlpha;
    return config;
}

PipelineConfig Pipeline::skyboxConfig(vk::RenderPass renderPass)
{
    PipelineConfig config;
    config.vertShaderPath = "skybox.vert.spv";
    config.fragShaderPath = "skybox.frag.spv";
    config.bindings = {
        {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
    };
    config.renderPass = renderPass;
    config.useVertexInput = false;
    config.depthWrite = false;
    config.depthCompare = vk::CompareOp::eLessOrEqual;
    config.cullMode = vk::CullModeFlagBits::eNone;
    return config;
}

void Pipeline::createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
    vk::DescriptorSetLayoutCreateInfo ci({}, bindings);
    descSetLayout_ = vk::raii::DescriptorSetLayout(*device_, ci);
}

void Pipeline::createGraphicsPipeline(const PipelineConfig& config)
{
    vk::raii::ShaderModule vertMod{nullptr};
    vk::raii::ShaderModule fragMod{nullptr};
    auto stages = createShaderStages(config, vertMod, fragMod);

    vk::VertexInputBindingDescription bindDesc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    std::array<vk::VertexInputAttributeDescription, 7> attrDesc = {{
        {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, position_))},
        {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, colour_))},
        {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, normal_))},
        {3, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(Vertex, texCoord_))},
        {4, 0, vk::Format::eR32G32B32A32Uint, static_cast<uint32_t>(offsetof(Vertex, joints_))},
        {5, 0, vk::Format::eR32G32B32A32Sfloat, static_cast<uint32_t>(offsetof(Vertex, weights_))},
        {6, 0, vk::Format::eR32G32B32A32Sfloat, static_cast<uint32_t>(offsetof(Vertex, tangent_))},
    }};
    vk::PipelineVertexInputStateCreateInfo vertInput;
    if (config.useVertexInput)
    {
        vertInput = vk::PipelineVertexInputStateCreateInfo({}, bindDesc, attrDesc);
    }

    vk::PipelineInputAssemblyStateCreateInfo inputAsm({}, vk::PrimitiveTopology::eTriangleList);

    vk::PipelineViewportStateCreateInfo vpState;
    vpState.viewportCount = 1;
    vpState.scissorCount = 1;

    std::array dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState({}, dynamicStates);

    vk::PipelineRasterizationStateCreateInfo raster(
        {}, false, false, vk::PolygonMode::eFill, config.cullMode,
        vk::FrontFace::eCounterClockwise, false, 0, 0, 0, 1.0f);

    vk::PipelineMultisampleStateCreateInfo ms({}, vk::SampleCountFlagBits::e1);

    vk::PipelineDepthStencilStateCreateInfo depthStencil({}, true, config.depthWrite,
                                                         config.depthCompare);

    vk::PipelineColorBlendAttachmentState colorBlendAtt(
        config.blendEnable, config.srcColorBlend, config.dstColorBlend, vk::BlendOp::eAdd,
        config.srcAlphaBlend, config.dstAlphaBlend, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, {}, colorBlendAtt);

    createPipelineLayout();

    vk::GraphicsPipelineCreateInfo pci({}, stages, &vertInput, &inputAsm, nullptr, &vpState,
                                       &raster, &ms, &depthStencil, &colorBlend, &dynamicState,
                                       *pipelineLayout_, config.renderPass, 0);

    pipeline_ = vk::raii::Pipeline(*device_, nullptr, pci);
}

std::array<vk::PipelineShaderStageCreateInfo, 2>
Pipeline::createShaderStages(const PipelineConfig& config, vk::raii::ShaderModule& vertMod,
                             vk::raii::ShaderModule& fragMod) const
{
    auto vertCode = ShaderLoader::load_from_file(config.vertShaderPath);
    auto fragCode = ShaderLoader::load_from_file(config.fragShaderPath);
    vk::ShaderModuleCreateInfo vertCi({}, vertCode.size(),
                                      reinterpret_cast<const uint32_t*>(vertCode.data()));
    vk::ShaderModuleCreateInfo fragCi({}, fragCode.size(),
                                      reinterpret_cast<const uint32_t*>(fragCode.data()));
    vertMod = vk::raii::ShaderModule(*device_, vertCi);
    fragMod = vk::raii::ShaderModule(*device_, fragCi);

    return {{
        {{}, vk::ShaderStageFlagBits::eVertex, *vertMod, "main"},
        {{}, vk::ShaderStageFlagBits::eFragment, *fragMod, "main"},
    }};
}

void Pipeline::createPipelineLayout()
{
    vk::PipelineLayoutCreateInfo plci({}, *descSetLayout_);
    pipelineLayout_ = vk::raii::PipelineLayout(*device_, plci);
}

} // namespace fire_engine
