#include <fire_engine/renderer/frame.hpp>
#include <fire_engine/renderer/ubo.hpp>

namespace fire_engine
{

Frame::Frame(const Device& device, Swapchain& swapchain)
    : device_(&device),
      swapchain_(&swapchain),
      vkDevice_(device.device())
{
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

Frame::~Frame()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDevice_.destroySemaphore(imageAvail_[i]);
        vkDevice_.destroyFence(inFlight_[i]);
    }
    vkDevice_.destroyCommandPool(cmdPool_);
    for (auto sem : renderDone_)
        vkDevice_.destroySemaphore(sem);
}

void Frame::createCommandPool()
{
    vk::CommandPoolCreateInfo ci(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                 device_->graphicsFamily());
    cmdPool_ = vkDevice_.createCommandPool(ci);
}

void Frame::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo ai(cmdPool_, vk::CommandBufferLevel::ePrimary,
                                     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
    cmdBufs_ = vkDevice_.allocateCommandBuffers(ai);
}

void Frame::createSyncObjects()
{
    imageAvail_.resize(MAX_FRAMES_IN_FLIGHT);
    renderDone_.resize(swapchain_->images().size());
    inFlight_.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo sci;
    vk::FenceCreateInfo fci(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        imageAvail_[i] = vkDevice_.createSemaphore(sci);
        inFlight_[i] = vkDevice_.createFence(fci);
    }
    for (size_t i = 0; i < swapchain_->images().size(); ++i)
    {
        renderDone_[i] = vkDevice_.createSemaphore(sci);
    }
}

void Frame::destroyRenderFinishedSemaphores()
{
    for (auto sem : renderDone_)
        vkDevice_.destroySemaphore(sem);
    renderDone_.clear();
}

void Frame::createRenderFinishedSemaphores(size_t count)
{
    vk::SemaphoreCreateInfo sci;
    renderDone_.resize(count);
    for (size_t i = 0; i < count; ++i)
        renderDone_[i] = vkDevice_.createSemaphore(sci);
}

} // namespace fire_engine
