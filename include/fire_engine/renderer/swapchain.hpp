#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/platform/window.hpp>
#include <fire_engine/renderer/device.hpp>

namespace fire_engine
{

class Swapchain
{
public:
    explicit Swapchain(const Device& device, const Window& window);
    ~Swapchain() = default;

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&) noexcept = default;
    Swapchain& operator=(Swapchain&&) noexcept = default;

    void createDepthResources(const Device& device);
    void createFramebuffers(vk::RenderPass renderPass);
    void recreate(const Device& device, const Window& window, vk::RenderPass renderPass);

    [[nodiscard]] vk::SwapchainKHR swapchain() const noexcept
    {
        return *swapchain_;
    }
    [[nodiscard]] const std::vector<vk::Image>& images() const noexcept
    {
        return images_;
    }
    [[nodiscard]] vk::Format format() const noexcept
    {
        return format_;
    }
    [[nodiscard]] vk::Extent2D extent() const noexcept
    {
        return extent_;
    }
    [[nodiscard]] vk::ImageView depthView() const noexcept
    {
        return *depthView_;
    }

    [[nodiscard]] vk::Framebuffer framebuffer(size_t index) const noexcept
    {
        return *framebuffers_[index];
    }
    [[nodiscard]] size_t framebufferCount() const noexcept
    {
        return framebuffers_.size();
    }

private:
    void createSwapchain(const Device& device, const Window& window);
    void createImageViews();

    [[nodiscard]] vk::SurfaceFormatKHR chooseSwapFormat(const Device& device);
    [[nodiscard]] vk::PresentModeKHR chooseSwapPresentMode(const Device& device);
    [[nodiscard]] vk::Extent2D chooseSwapExtent(const Window& window,
                                                const vk::SurfaceCapabilitiesKHR& caps);
    [[nodiscard]] vk::raii::ImageView createImageView(vk::Image img, vk::Format fmt,
                                                      vk::ImageAspectFlags aspect);

    const vk::raii::Device* device_{nullptr};
    vk::raii::SwapchainKHR swapchain_{nullptr};
    std::vector<vk::Image> images_;
    std::vector<vk::raii::ImageView> views_;
    vk::Format format_{};
    vk::Extent2D extent_{};

    vk::raii::Image depthImage_{nullptr};
    vk::raii::DeviceMemory depthMem_{nullptr};
    vk::raii::ImageView depthView_{nullptr};
    std::vector<vk::raii::Framebuffer> framebuffers_;
};

} // namespace fire_engine
