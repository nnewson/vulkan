#include <fire_engine/render/render_pass.hpp>

#include <array>

#include <fire_engine/render/device.hpp>
#include <fire_engine/render/swapchain.hpp>

namespace fire_engine
{

RenderPass RenderPass::createForward(const Device& device, const Swapchain& swapchain)
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

    RenderPass pass;
    pass.renderPass_ = vk::raii::RenderPass(device.device(), ci);
    return pass;
}

RenderPass RenderPass::createShadow(const Device& device)
{
    vk::AttachmentDescription depthAtt(
        {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference depthRef(0, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, {}, {}, &depthRef);

    // Gate shadow writes behind previous-frame forward reads, and drive the
    // implicit transition to shader-read-only on pass end so the forward
    // fragment shader sees a sampleable depth image.
    std::array<vk::SubpassDependency, 2> deps{
        vk::SubpassDependency{VK_SUBPASS_EXTERNAL, 0,
                              vk::PipelineStageFlagBits::eFragmentShader,
                              vk::PipelineStageFlagBits::eEarlyFragmentTests,
                              vk::AccessFlagBits::eShaderRead,
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite},
        vk::SubpassDependency{0, VK_SUBPASS_EXTERNAL,
                              vk::PipelineStageFlagBits::eLateFragmentTests,
                              vk::PipelineStageFlagBits::eFragmentShader,
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                              vk::AccessFlagBits::eShaderRead},
    };

    vk::RenderPassCreateInfo ci({}, depthAtt, subpass, deps);

    RenderPass pass;
    pass.renderPass_ = vk::raii::RenderPass(device.device(), ci);
    return pass;
}

void RenderPass::createShadowFramebuffer(const Device& device, vk::ImageView depthView,
                                         uint32_t extent)
{
    framebuffers_.clear();
    vk::FramebufferCreateInfo ci({}, *renderPass_, depthView, extent, extent, 1);
    framebuffers_.emplace_back(device.device(), ci);
}

void RenderPass::createForwardFramebuffers(const Device& device, const Swapchain& swapchain)
{
    framebuffers_.clear();

    auto extent = swapchain.extent();
    const auto& colorViews = swapchain.imageViews();
    framebuffers_.reserve(colorViews.size());

    for (std::size_t i = 0; i < colorViews.size(); ++i)
    {
        std::array<vk::ImageView, 2> attachments = {*colorViews[i], swapchain.depthView()};
        vk::FramebufferCreateInfo ci({}, *renderPass_, attachments, extent.width, extent.height, 1);
        framebuffers_.emplace_back(device.device(), ci);
    }
}

} // namespace fire_engine
