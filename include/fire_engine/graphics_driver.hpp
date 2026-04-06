#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include <fire_engine/platform/window.hpp>
#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/texture.hpp>
#include <fire_engine/renderer/renderer.hpp>

namespace fire_engine
{

class GraphicsDriver
{
public:
    explicit GraphicsDriver(Renderer& renderer);
    ~GraphicsDriver();

    void init();
    void drawFrame(Window& display, Vec3 cameraPos, Vec3 cameraTarget);
    void waitIdle();

private:
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPool();
    void createGeometryBuffer();
    void createTexture();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    vk::ShaderModule createShaderModule(const std::vector<char>& code);
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags props, vk::Buffer& buf, vk::DeviceMemory& mem);
    void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex);
    void updateUniformBuffer(Vec3 cameraPos, Vec3 cameraTarget);
    void recreateSwapchain(const Window& display);

    Renderer* renderer_;

    vk::RenderPass renderPass_;
    vk::DescriptorSetLayout descSetLayout_;
    vk::PipelineLayout pipelineLayout_;
    vk::Pipeline pipeline_;

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
    uint32_t currentFrame_ = 0;
};

} // namespace fire_engine
