#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include <fire_engine/renderer/device.hpp>
#include <fire_engine/renderer/swapchain.hpp>

namespace fire_engine
{

class Frame
{
public:
    Frame(const Device& device, Swapchain& swapchain);
    ~Frame();

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame(Frame&&) noexcept = default;
    Frame& operator=(Frame&&) noexcept = default;

    [[nodiscard]] vk::CommandBuffer commandBuffer(uint32_t index) const noexcept
    {
        return cmdBufs_[index];
    }

    [[nodiscard]] vk::Semaphore imageAvailable(uint32_t index) const noexcept
    {
        return imageAvail_[index];
    }

    [[nodiscard]] vk::Semaphore renderFinished(uint32_t imageIndex) const noexcept
    {
        return renderDone_[imageIndex];
    }

    [[nodiscard]] vk::Fence inFlightFence(uint32_t index) const noexcept
    {
        return inFlight_[index];
    }

    [[nodiscard]] vk::CommandPool commandPool() const noexcept
    {
        return cmdPool_;
    }

    void destroyRenderFinishedSemaphores();
    void createRenderFinishedSemaphores(size_t count);

private:
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    const Device* device_;
    Swapchain* swapchain_;
    vk::Device vkDevice_;

    vk::CommandPool cmdPool_;
    std::vector<vk::CommandBuffer> cmdBufs_;

    std::vector<vk::Semaphore> imageAvail_;
    std::vector<vk::Semaphore> renderDone_;
    std::vector<vk::Fence> inFlight_;
};

} // namespace fire_engine
