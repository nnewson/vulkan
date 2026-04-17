#include <cstddef>

#include <fire_engine/core/shader_loader.hpp>
#include <fire_engine/graphics/vertex.hpp>
#include <fire_engine/render/pipeline.hpp>

namespace fire_engine
{

Pipeline::Pipeline(const Device& device, const Swapchain& swapchain, const PipelineConfig& config)
    : device_(&device.device())
{
    createDescriptorSetLayout(config.bindings);
    createGraphicsPipeline(swapchain, config);
}

vk::raii::RenderPass Pipeline::createForwardRenderPass(const Device& device,
                                                       const Swapchain& swapchain)
{
    vk::AttachmentDescription colorAtt(
        {}, swapchain.format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentDescription depthAtt(
        {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colorRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colorRef, {},
                                   &depthRef);

    vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests,
                              {},
                              vk::AccessFlagBits::eColorAttachmentWrite |
                                  vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array<vk::AttachmentDescription, 2> attachments = {colorAtt, depthAtt};
    vk::RenderPassCreateInfo ci({}, attachments, subpass, dep);
    return vk::raii::RenderPass(device.device(), ci);
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
    };
    config.renderPass = renderPass;
    return config;
}

void Pipeline::createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
    vk::DescriptorSetLayoutCreateInfo ci({}, bindings);
    descSetLayout_ = vk::raii::DescriptorSetLayout(*device_, ci);
}

void Pipeline::createGraphicsPipeline(const Swapchain& swapchain, const PipelineConfig& config)
{
    auto extent = swapchain.extent();

    vk::raii::ShaderModule vertMod{nullptr};
    vk::raii::ShaderModule fragMod{nullptr};
    auto stages = createShaderStages(config, vertMod, fragMod);

    vk::VertexInputBindingDescription bindDesc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    std::array<vk::VertexInputAttributeDescription, 6> attrDesc = {{
        {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, position_))},
        {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, colour_))},
        {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, normal_))},
        {3, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(Vertex, texCoord_))},
        {4, 0, vk::Format::eR32G32B32A32Uint, static_cast<uint32_t>(offsetof(Vertex, joints_))},
        {5, 0, vk::Format::eR32G32B32A32Sfloat, static_cast<uint32_t>(offsetof(Vertex, weights_))},
    }};
    vk::PipelineVertexInputStateCreateInfo vertInput({}, bindDesc, attrDesc);

    vk::PipelineInputAssemblyStateCreateInfo inputAsm({}, vk::PrimitiveTopology::eTriangleList);

    vk::Viewport viewport(0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height),
                          0, 1);
    vk::Rect2D scissor({0, 0}, extent);
    vk::PipelineViewportStateCreateInfo vpState({}, viewport, scissor);

    vk::PipelineRasterizationStateCreateInfo raster(
        {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise, false, 0, 0, 0, 1.0f);

    vk::PipelineMultisampleStateCreateInfo ms({}, vk::SampleCountFlagBits::e1);

    vk::PipelineDepthStencilStateCreateInfo depthStencil({}, true, true, vk::CompareOp::eLess);

    vk::PipelineColorBlendAttachmentState colorBlendAtt(
        false, {}, {}, {}, {}, {}, {},
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, {}, colorBlendAtt);

    createPipelineLayout();

    vk::GraphicsPipelineCreateInfo pci({}, stages, &vertInput, &inputAsm, nullptr, &vpState,
                                       &raster, &ms, &depthStencil, &colorBlend, nullptr,
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
