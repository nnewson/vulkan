#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

#include <fire_engine/platform/window.hpp>
#include <fire_engine/graphics/geometry.hpp>
#include <fire_engine/graphics/texture.hpp>

namespace fire_engine
{

class GraphicsDriver
{
public:
    explicit GraphicsDriver();
    ~GraphicsDriver();

    void init(const Window& display);
    void drawFrame(const Window& display, Vec3 cameraPos, Vec3 cameraTarget);
    void waitIdle();
    void framebufferResized();

private:
    void createSurface(const Window& display);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain(const Window& display);
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createDepthResources();
    void createFramebuffers();
    void createCommandPool();
    void createGeometryBuffer();
    void createTexture();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    bool isDeviceSuitable(vk::PhysicalDevice d);
    std::pair<std::optional<uint32_t>, std::optional<uint32_t>>
    findQueueFamilies(vk::PhysicalDevice d);

    vk::SurfaceFormatKHR chooseSwapFormat();
    vk::PresentModeKHR chooseSwapPresentMode();
    vk::Extent2D chooseSwapExtent(const Window& display, const vk::SurfaceCapabilitiesKHR& caps);

    vk::ShaderModule createShaderModule(const std::vector<char>& code);
    uint32_t findMemoryType(uint32_t filter, vk::MemoryPropertyFlags props);
    vk::ImageView createImageView(vk::Image img, vk::Format fmt, vk::ImageAspectFlags aspect);
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags props, vk::Buffer& buf, vk::DeviceMemory& mem);
    void recordCommandBuffer(vk::CommandBuffer cmd, uint32_t imageIndex);
    void updateUniformBuffer(Vec3 cameraPos, Vec3 cameraTarget);
    void cleanupSwapchain();
    void recreateSwapchain(const Window& display);

    vk::Instance instance_;
    vk::SurfaceKHR surface_;

    vk::PhysicalDevice physDevice_;
    vk::Device device_;
    vk::Queue graphicsQueue_;
    vk::Queue presentQueue_;
    uint32_t graphicsFamily_ = 0;
    uint32_t presentFamily_ = 0;

    vk::SwapchainKHR swapchain_;
    std::vector<vk::Image> swapImages_;
    std::vector<vk::ImageView> swapViews_;
    vk::Format swapFormat_{};
    vk::Extent2D swapExtent_{};

    vk::RenderPass renderPass_;
    vk::DescriptorSetLayout descSetLayout_;
    vk::PipelineLayout pipelineLayout_;
    vk::Pipeline pipeline_;

    std::vector<vk::Framebuffer> framebuffers_;

    vk::Image depthImage_;
    vk::DeviceMemory depthMem_;
    vk::ImageView depthView_;

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
    bool framebufferResized_ = false;
};

} // namespace fire_engine
