#include <algorithm>

#include <fire_engine/renderer/swapchain.hpp>

namespace fire_engine
{

Swapchain::Swapchain(const Device& device, const Window& window)
    : device_(&device.device())
{
    createSwapchain(device, window);
    createImageViews();
}

void Swapchain::createDepthResources(const Device& device)
{
    vk::Format depthFmt = vk::Format::eD32Sfloat;
    vk::ImageCreateInfo ci({}, vk::ImageType::e2D, depthFmt,
                           vk::Extent3D(extent_.width, extent_.height, 1), 1, 1,
                           vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                           vk::ImageUsageFlagBits::eDepthStencilAttachment,
                           vk::SharingMode::eExclusive, {}, vk::ImageLayout::eUndefined);
    depthImage_ = vk::raii::Image(*device_, ci);

    auto req = depthImage_.getMemoryRequirements();
    vk::MemoryAllocateInfo ai(
        req.size,
        device.findMemoryType(req.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    depthMem_ = vk::raii::DeviceMemory(*device_, ai);
    depthImage_.bindMemory(*depthMem_, 0);
    depthView_ = createImageView(*depthImage_, depthFmt, vk::ImageAspectFlagBits::eDepth);
}

void Swapchain::createFramebuffers(vk::RenderPass renderPass)
{
    framebuffers_.clear();
    framebuffers_.reserve(views_.size());
    for (size_t i = 0; i < views_.size(); ++i)
    {
        std::array<vk::ImageView, 2> attachments = {*views_[i], *depthView_};
        vk::FramebufferCreateInfo ci({}, renderPass, attachments, extent_.width, extent_.height, 1);
        framebuffers_.emplace_back(*device_, ci);
    }
}

void Swapchain::recreate(const Device& device, const Window& window, vk::RenderPass renderPass)
{
    // Destroy in reverse dependency order
    framebuffers_.clear();
    views_.clear();
    depthView_ = nullptr;
    depthImage_ = nullptr;
    depthMem_ = nullptr;
    swapchain_ = nullptr;

    createSwapchain(device, window);
    createImageViews();
    createDepthResources(device);
    createFramebuffers(renderPass);
}

void Swapchain::createSwapchain(const Device& device, const Window& window)
{
    auto caps = device.physicalDevice().getSurfaceCapabilitiesKHR(*device.surface());
    auto fmt = chooseSwapFormat(device);
    auto mode = chooseSwapPresentMode(device);
    auto extent = chooseSwapExtent(window, caps);

    uint32_t imgCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0)
        imgCount = std::min(imgCount, caps.maxImageCount);

    uint32_t families[] = {device.graphicsFamily(), device.presentFamily()};
    bool concurrent = device.graphicsFamily() != device.presentFamily();

    vk::SwapchainCreateInfoKHR ci(
        {}, *device.surface(), imgCount, fmt.format, fmt.colorSpace, extent, 1,
        vk::ImageUsageFlagBits::eColorAttachment,
        concurrent ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
        concurrent ? 2u : 0u, concurrent ? families : nullptr, caps.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque, mode, vk::True);

    swapchain_ = vk::raii::SwapchainKHR(*device_, ci);
    images_ = swapchain_.getImages();
    format_ = fmt.format;
    extent_ = extent;
}

void Swapchain::createImageViews()
{
    views_.clear();
    views_.reserve(images_.size());
    for (size_t i = 0; i < images_.size(); ++i)
        views_.push_back(createImageView(images_[i], format_, vk::ImageAspectFlagBits::eColor));
}

vk::SurfaceFormatKHR Swapchain::chooseSwapFormat(const Device& device)
{
    auto fmts = device.physicalDevice().getSurfaceFormatsKHR(*device.surface());
    for (auto& f : fmts)
        if (f.format == vk::Format::eB8G8R8A8Srgb &&
            f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return f;
    return fmts[0];
}

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const Device& device)
{
    auto modes = device.physicalDevice().getSurfacePresentModesKHR(*device.surface());
    for (auto& m : modes)
        if (m == vk::PresentModeKHR::eMailbox)
            return m;
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseSwapExtent(const Window& window,
                                         const vk::SurfaceCapabilitiesKHR& caps)
{
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;
    auto [w, h] = window.framebufferSize();
    vk::Extent2D ext(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    ext.width = std::clamp(ext.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    ext.height = std::clamp(ext.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return ext;
}

vk::raii::ImageView Swapchain::createImageView(vk::Image img, vk::Format fmt,
                                               vk::ImageAspectFlags aspect)
{
    vk::ImageViewCreateInfo ci({}, img, vk::ImageViewType::e2D, fmt, {},
                               vk::ImageSubresourceRange(aspect, 0, 1, 0, 1));
    return vk::raii::ImageView(*device_, ci);
}

} // namespace fire_engine
