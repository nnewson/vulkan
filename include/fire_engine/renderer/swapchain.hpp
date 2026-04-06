#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include <fire_engine/platform/window.hpp>
#include <fire_engine/renderer/device.hpp>

namespace fire_engine
{

class Swapchain
{
public:
    explicit Swapchain(const Device& device, const Window& window);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&) noexcept = default;
    Swapchain& operator=(Swapchain&&) noexcept = default;

    void createDepthResources(const Device& device);
    void createFramebuffers(vk::RenderPass renderPass);
    void cleanup();
    void recreate(const Device& device, const Window& window, vk::RenderPass renderPass);

    [[nodiscard]] vk::SwapchainKHR swapchain() const noexcept
    {
        return swapchain_;
    }
    [[nodiscard]] const std::vector<vk::Image>& images() const noexcept
    {
        return images_;
    }
    [[nodiscard]] const std::vector<vk::ImageView>& views() const noexcept
    {
        return views_;
    }
    [[nodiscard]] vk::Format format() const noexcept
    {
        return format_;
    }
    [[nodiscard]] vk::Extent2D extent() const noexcept
    {
        return extent_;
    }
    [[nodiscard]] const std::vector<vk::Framebuffer>& framebuffers() const noexcept
    {
        return framebuffers_;
    }
    [[nodiscard]] vk::ImageView depthView() const noexcept
    {
        return depthView_;
    }

private:
    void createSwapchain(const Device& device, const Window& window);
    void createImageViews();

    [[nodiscard]] vk::SurfaceFormatKHR chooseSwapFormat(const Device& device);
    [[nodiscard]] vk::PresentModeKHR chooseSwapPresentMode(const Device& device);
    [[nodiscard]] vk::Extent2D chooseSwapExtent(const Window& window,
                                                const vk::SurfaceCapabilitiesKHR& caps);
    [[nodiscard]] vk::ImageView createImageView(vk::Image img, vk::Format fmt,
                                                vk::ImageAspectFlags aspect);

    vk::Device device_; // stored for cleanup
    vk::SwapchainKHR swapchain_;
    std::vector<vk::Image> images_;
    std::vector<vk::ImageView> views_;
    vk::Format format_{};
    vk::Extent2D extent_{};

    vk::Image depthImage_;
    vk::DeviceMemory depthMem_;
    vk::ImageView depthView_;
    std::vector<vk::Framebuffer> framebuffers_;
};

} // namespace fire_engine
