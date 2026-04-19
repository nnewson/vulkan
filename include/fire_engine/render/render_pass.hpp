#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

namespace fire_engine
{

class Device;
class Swapchain;

// Owns a VkRenderPass and the framebuffers that render into it. A single
// logical pass (forward, shadow, post-process) is represented by one
// RenderPass instance. Framebuffers may be backed by swapchain images
// (forward) or by offscreen attachments (shadow/post-process).
class RenderPass
{
public:
    RenderPass() = default;
    ~RenderPass() = default;

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) noexcept = default;
    RenderPass& operator=(RenderPass&&) noexcept = default;

    // Creates the forward-lit render pass (color + depth, presents to
    // swapchain). Framebuffers are not yet built — call
    // createForwardFramebuffers once the swapchain has depth resources.
    [[nodiscard]] static RenderPass createForward(const Device& device,
                                                  const Swapchain& swapchain);

    // Builds (or rebuilds, after a swapchain resize) framebuffers using the
    // swapchain's color views and depth view. Safe to call repeatedly.
    void createForwardFramebuffers(const Device& device, const Swapchain& swapchain);

    // Creates a depth-only shadow render pass. A single D32_SFLOAT attachment
    // with eClear loadOp / eStore storeOp and a finalLayout of
    // eShaderReadOnlyOptimal so the implicit transition exposes the depth map
    // to the forward fragment shader without an explicit pipeline barrier.
    [[nodiscard]] static RenderPass createShadow(const Device& device);

    // Builds a single framebuffer over an offscreen depth view, sized by
    // extent. Owned framebuffer list is replaced on every call.
    void createShadowFramebuffer(const Device& device, vk::ImageView depthView,
                                 uint32_t extent);

    [[nodiscard]] vk::RenderPass renderPass() const noexcept
    {
        return *renderPass_;
    }

    [[nodiscard]] vk::Framebuffer framebuffer(std::size_t index) const noexcept
    {
        return *framebuffers_[index];
    }

    [[nodiscard]] std::size_t framebufferCount() const noexcept
    {
        return framebuffers_.size();
    }

private:
    vk::raii::RenderPass renderPass_{nullptr};
    std::vector<vk::raii::Framebuffer> framebuffers_;
};

} // namespace fire_engine
