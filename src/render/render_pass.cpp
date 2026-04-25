#include <fire_engine/render/render_pass.hpp>

#include <array>

#include <fire_engine/render/device.hpp>
#include <fire_engine/render/swapchain.hpp>

namespace fire_engine
{

RenderPass RenderPass::createForward(const Device& device)
{
    // Forward writes into an offscreen HDR target (post-process reads it).
    vk::AttachmentDescription colourAtt(
        {}, vk::Format::eR16G16B16A16Sfloat, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentDescription depthAtt(
        {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colourRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colourRef, {},
                                   &depthRef);

    std::array<vk::SubpassDependency, 2> deps = {
        vk::SubpassDependency(VK_SUBPASS_EXTERNAL, 0,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests,
                              {},
                              vk::AccessFlagBits::eColorAttachmentWrite |
                                  vk::AccessFlagBits::eDepthStencilAttachmentWrite),
        vk::SubpassDependency(
            0, VK_SUBPASS_EXTERNAL, vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead),
    };

    std::array<vk::AttachmentDescription, 2> attachments = {colourAtt, depthAtt};
    vk::RenderPassCreateInfo ci({}, attachments, subpass, deps);

    RenderPass pass;
    pass.renderPass_ = vk::raii::RenderPass(device.device(), ci);
    return pass;
}

RenderPass RenderPass::createShadow(const Device& device)
{
    // MoltenVK quirk: depth-only render passes don't commit storeOp on TBDR.
    // Attach a throwaway B8G8R8A8 colour target so Metal treats this as a real
    // render pass. loadOp=DontCare/storeOp=DontCare keeps it free.
    std::array<vk::AttachmentDescription, 2> attachments = {
        vk::AttachmentDescription({}, vk::Format::eB8G8R8A8Unorm, vk::SampleCountFlagBits::e1,
                                  vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                                  vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eColorAttachmentOptimal),
        vk::AttachmentDescription({}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
                                  vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                                  vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eDepthStencilReadOnlyOptimal),
    };

    vk::AttachmentReference colourRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colourRef, {},
                                   &depthRef);

    std::array<vk::SubpassDependency, 2> deps = {
        vk::SubpassDependency(VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eFragmentShader,
                              vk::PipelineStageFlagBits::eEarlyFragmentTests,
                              vk::AccessFlagBits::eShaderRead,
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite),
        vk::SubpassDependency(0, VK_SUBPASS_EXTERNAL, vk::PipelineStageFlagBits::eLateFragmentTests,
                              vk::PipelineStageFlagBits::eFragmentShader,
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                              vk::AccessFlagBits::eShaderRead),
    };
    vk::RenderPassCreateInfo ci({}, attachments, subpass, deps);

    RenderPass pass;
    pass.renderPass_ = vk::raii::RenderPass(device.device(), ci);
    return pass;
}

void RenderPass::createShadowFramebuffer(const Device& device, vk::ImageView colourView,
                                         std::span<const vk::ImageView> depthViews, uint32_t extent)
{
    framebuffers_.clear();
    framebuffers_.reserve(depthViews.size());
    for (vk::ImageView depthView : depthViews)
    {
        std::array<vk::ImageView, 2> views = {colourView, depthView};
        vk::FramebufferCreateInfo ci({}, *renderPass_, views, extent, extent, 1);
        framebuffers_.emplace_back(device.device(), ci);
    }
}

void RenderPass::createForwardFramebuffer(const Device& device, vk::ImageView colourView,
                                          vk::ImageView depthView, vk::Extent2D extent)
{
    framebuffers_.clear();
    std::array<vk::ImageView, 2> attachments = {colourView, depthView};
    vk::FramebufferCreateInfo ci({}, *renderPass_, attachments, extent.width, extent.height, 1);
    framebuffers_.emplace_back(device.device(), ci);
}

RenderPass RenderPass::createBloomDown(const Device& device, vk::Format colourFormat)
{
    vk::AttachmentDescription colourAtt(
        {}, colourFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference colourRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colourRef);

    vk::SubpassDependency dep(
        VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eShaderRead,
        vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo ci({}, colourAtt, subpass, dep);
    RenderPass pass;
    pass.renderPass_ = vk::raii::RenderPass(device.device(), ci);
    return pass;
}

RenderPass RenderPass::createBloomUp(const Device& device, vk::Format colourFormat)
{
    // initialLayout=eShaderReadOnlyOptimal because the downsample pass left
    // every mip in that layout. loadOp=Load preserves it; the upsample pipeline
    // uses additive blending on top.
    vk::AttachmentDescription colourAtt(
        {}, colourFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eLoad,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference colourRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colourRef);

    vk::SubpassDependency dep(
        VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eShaderRead,
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo ci({}, colourAtt, subpass, dep);
    RenderPass pass;
    pass.renderPass_ = vk::raii::RenderPass(device.device(), ci);
    return pass;
}

void RenderPass::createColourFramebuffersPerMip(const Device& device,
                                                std::span<const vk::ImageView> colourViews,
                                                std::span<const vk::Extent2D> extents)
{
    framebuffers_.clear();
    framebuffers_.reserve(colourViews.size());
    for (std::size_t i = 0; i < colourViews.size(); ++i)
    {
        vk::ImageView v = colourViews[i];
        vk::FramebufferCreateInfo ci({}, *renderPass_, v, extents[i].width, extents[i].height, 1);
        framebuffers_.emplace_back(device.device(), ci);
    }
}

RenderPass RenderPass::createPostProcess(const Device& device, const Swapchain& swapchain)
{
    vk::AttachmentDescription colourAtt(
        {}, swapchain.format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colourRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colourRef);

    // Wait for the forward pass to finish writing the HDR target before the
    // post-process fragment shader samples it.
    vk::SubpassDependency dep(
        VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead);

    vk::RenderPassCreateInfo ci({}, colourAtt, subpass, dep);
    RenderPass pass;
    pass.renderPass_ = vk::raii::RenderPass(device.device(), ci);
    return pass;
}

void RenderPass::createPostProcessFramebuffers(const Device& device, const Swapchain& swapchain)
{
    framebuffers_.clear();

    auto extent = swapchain.extent();
    const auto& colourViews = swapchain.imageViews();
    framebuffers_.reserve(colourViews.size());

    for (std::size_t i = 0; i < colourViews.size(); ++i)
    {
        vk::ImageView view = *colourViews[i];
        vk::FramebufferCreateInfo ci({}, *renderPass_, view, extent.width, extent.height, 1);
        framebuffers_.emplace_back(device.device(), ci);
    }
}

RenderPass RenderPass::createOffscreenColour(const Device& device, vk::Format colourFormat)
{
    vk::AttachmentDescription colourAtt(
        {}, colourFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference colourRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colourRef);

    std::array<vk::SubpassDependency, 2> deps = {
        vk::SubpassDependency(VK_SUBPASS_EXTERNAL, 0,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput, {},
                              vk::AccessFlagBits::eColorAttachmentWrite),
        vk::SubpassDependency(
            0, VK_SUBPASS_EXTERNAL, vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead),
    };

    vk::RenderPassCreateInfo ci({}, colourAtt, subpass, deps);
    RenderPass pass;
    pass.renderPass_ = vk::raii::RenderPass(device.device(), ci);
    return pass;
}

void RenderPass::createColourFramebuffers(const Device& device,
                                          std::span<const vk::ImageView> colourViews,
                                          uint32_t extent)
{
    framebuffers_.clear();
    framebuffers_.reserve(colourViews.size());

    for (vk::ImageView colourView : colourViews)
    {
        vk::FramebufferCreateInfo ci({}, *renderPass_, colourView, extent, extent, 1);
        framebuffers_.emplace_back(device.device(), ci);
    }
}

} // namespace fire_engine
