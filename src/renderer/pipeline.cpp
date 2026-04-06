#include <fire_engine/core/shader_loader.hpp>
#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/renderer/pipeline.hpp>

namespace fire_engine
{

Pipeline::Pipeline(const Device& device, const Swapchain& swapchain)
    : device_(device.device())
{
    createRenderPass(swapchain);
    createDescriptorSetLayout();
    createGraphicsPipeline(swapchain);
}

Pipeline::~Pipeline()
{
    device_.destroyPipeline(pipeline_);
    device_.destroyPipelineLayout(pipelineLayout_);
    device_.destroyRenderPass(renderPass_);
    device_.destroyDescriptorSetLayout(descSetLayout_);
}

void Pipeline::createRenderPass(const Swapchain& swapchain)
{
    vk::AttachmentDescription colorAtt(
        {}, swapchain.format(), vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

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
    renderPass_ = device_.createRenderPass(ci);
}

void Pipeline::createDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {{
        {0, vk::DescriptorType::eUniformBuffer, 1,
         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
        {1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
        {2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
    }};

    vk::DescriptorSetLayoutCreateInfo ci({}, bindings);
    descSetLayout_ = device_.createDescriptorSetLayout(ci);
}

vk::ShaderModule Pipeline::createShaderModule(const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo ci({}, code.size(), reinterpret_cast<const uint32_t*>(code.data()));
    return device_.createShaderModule(ci);
}

void Pipeline::createGraphicsPipeline(const Swapchain& swapchain)
{
    auto extent = swapchain.extent();

    auto vertCode = ShaderLoader::load_from_file("shader.vert.spv");
    auto fragCode = ShaderLoader::load_from_file("shader.frag.spv");
    vk::ShaderModule vertMod = createShaderModule(vertCode);
    vk::ShaderModule fragMod = createShaderModule(fragCode);

    std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {{
        {{}, vk::ShaderStageFlagBits::eVertex, vertMod, "main"},
        {{}, vk::ShaderStageFlagBits::eFragment, fragMod, "main"},
    }};

    auto bindDesc = Vertex::bindingDesc();
    auto attrDesc = Vertex::attrDescs();
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

    vk::PipelineLayoutCreateInfo plci({}, descSetLayout_);
    pipelineLayout_ = device_.createPipelineLayout(plci);

    vk::GraphicsPipelineCreateInfo pci({}, stages, &vertInput, &inputAsm, nullptr, &vpState,
                                       &raster, &ms, &depthStencil, &colorBlend, nullptr,
                                       pipelineLayout_, renderPass_, 0);

    auto result = device_.createGraphicsPipeline(nullptr, pci);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("failed to create graphics pipeline");
    pipeline_ = result.value;

    device_.destroyShaderModule(fragMod);
    device_.destroyShaderModule(vertMod);
}

} // namespace fire_engine
