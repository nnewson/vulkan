#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/material.hpp>
#include <fire_engine/graphics/texture.hpp>
#include <fire_engine/renderer/device.hpp>
#include <fire_engine/renderer/pipeline.hpp>
#include <fire_engine/renderer/swapchain.hpp>

namespace fire_engine
{

class Frame
{
public:
    Frame(const Device& device, Swapchain& swapchain, const Pipeline& pipeline);
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

    [[nodiscard]] vk::DescriptorSet descriptorSet(uint32_t index) const noexcept
    {
        return descSets_[index];
    }

    [[nodiscard]] const Geometry::IndexedRenderData& renderData() const noexcept
    {
        return renderData_;
    }

    [[nodiscard]] vk::Buffer vertexBuffer() const noexcept
    {
        return vertexBuf_;
    }

    [[nodiscard]] vk::Buffer indexBuffer() const noexcept
    {
        return indexBuf_;
    }

    [[nodiscard]] void* uniformMapped(uint32_t index) const noexcept
    {
        return uniformMapped_[index];
    }

    void destroyRenderFinishedSemaphores();
    void createRenderFinishedSemaphores(size_t count);

private:
    void createCommandPool();
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags props, vk::Buffer& buf, vk::DeviceMemory& mem);
    void createGeometryBuffer();
    void createTexture();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    const Device* device_;
    Swapchain* swapchain_;
    const Pipeline* pipeline_;
    vk::Device vkDevice_;

    Geometry::IndexedRenderData renderData_;
    Material material_;
    Texture texture_;

    vk::Buffer vertexBuf_;
    vk::DeviceMemory vertexMem_;
    vk::Buffer indexBuf_;
    vk::DeviceMemory indexMem_;

    std::vector<vk::Buffer> uniformBufs_;
    std::vector<vk::DeviceMemory> uniformMems_;
    std::vector<void*> uniformMapped_;

    std::vector<vk::Buffer> materialBufs_;
    std::vector<vk::DeviceMemory> materialMems_;
    std::vector<void*> materialMapped_;

    vk::DescriptorPool descPool_;
    std::vector<vk::DescriptorSet> descSets_;

    vk::CommandPool cmdPool_;
    std::vector<vk::CommandBuffer> cmdBufs_;

    std::vector<vk::Semaphore> imageAvail_;
    std::vector<vk::Semaphore> renderDone_;
    std::vector<vk::Fence> inFlight_;
};

} // namespace fire_engine
