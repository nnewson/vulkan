#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/graphics/gpu_handle.hpp>
#include <fire_engine/render/constants.hpp>

namespace fire_engine
{

class Device;
class Image;
class Pipeline;
class Vertex;

class Resources
{
public:
    Resources(const Device& device, const Pipeline& pipeline);
    ~Resources() = default;

    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources(Resources&&) noexcept = default;
    Resources& operator=(Resources&&) noexcept = default;

    // --- Buffer creation ---

    [[nodiscard]] BufferHandle createVertexBuffer(std::span<const Vertex> vertices);
    [[nodiscard]] BufferHandle createIndexBuffer(std::span<const uint16_t> indices);

    // --- Texture creation ---

    [[nodiscard]] TextureHandle createTexture(const Image& image);
    [[nodiscard]] TextureHandle createTexture(const uint8_t* pixels, int width, int height);

    // --- Mapped buffer sets (per-frame, for UBOs and SSBOs) ---

    struct MappedBufferSet
    {
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> buffers{NullBuffer, NullBuffer};
        std::array<MappedMemory, MAX_FRAMES_IN_FLIGHT> mapped{};
    };

    [[nodiscard]] MappedBufferSet createMappedUniformBuffers(std::size_t size);
    [[nodiscard]] MappedBufferSet createMappedStorageBuffer(std::size_t size,
                                                            const void* initialData);

    // --- Descriptor sets ---

    struct GeometryDescriptorInfo
    {
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> materialBufs{NullBuffer, NullBuffer};
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> skinBufs{NullBuffer, NullBuffer};
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> morphUboBufs{NullBuffer, NullBuffer};
        BufferHandle morphSsbo{NullBuffer};
        std::size_t morphSsboSize{0};
        TextureHandle texture{NullTexture};
    };

    struct ObjectDescriptorRequest
    {
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> uniformBufs{NullBuffer, NullBuffer};
        std::vector<GeometryDescriptorInfo> geometries;
    };

    struct ObjectDescriptorResult
    {
        // descSets[geometryIndex][frameIndex]
        std::vector<std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>> descSets;
    };

    [[nodiscard]] ObjectDescriptorResult
    createObjectDescriptors(const ObjectDescriptorRequest& req);

    // --- Pipeline registry ---
    // Pipelines are owned elsewhere (by Pipeline class); Resources stores raw
    // handles so Renderer can resolve PipelineHandle values stamped on
    // DrawCommands to the Vulkan pipeline/layout pair to bind.

    [[nodiscard]] PipelineHandle registerPipeline(vk::Pipeline pipeline,
                                                  vk::PipelineLayout layout);

    // --- Vulkan accessors (for Renderer command recording) ---

    [[nodiscard]] vk::Buffer vulkanBuffer(BufferHandle handle) const noexcept;
    [[nodiscard]] vk::ImageView vulkanImageView(TextureHandle handle) const noexcept;
    [[nodiscard]] vk::Sampler vulkanSampler(TextureHandle handle) const noexcept;
    [[nodiscard]] vk::DescriptorSet vulkanDescriptorSet(DescriptorSetHandle handle) const noexcept;
    [[nodiscard]] vk::Pipeline vulkanPipeline(PipelineHandle handle) const noexcept;
    [[nodiscard]] vk::PipelineLayout
    vulkanPipelineLayout(PipelineHandle handle) const noexcept;

private:
    BufferHandle storeBuffer(vk::raii::Buffer buf, vk::raii::DeviceMemory mem);

    const Device* device_;
    const Pipeline* pipeline_;

    vk::raii::CommandPool cmdPool_{nullptr};

    struct BufferEntry
    {
        vk::raii::Buffer buffer{nullptr};
        vk::raii::DeviceMemory memory{nullptr};
    };
    std::vector<BufferEntry> buffers_;

    struct TextureEntry
    {
        vk::raii::Image image{nullptr};
        vk::raii::DeviceMemory memory{nullptr};
        vk::raii::ImageView view{nullptr};
        vk::raii::Sampler sampler{nullptr};
    };
    std::vector<TextureEntry> textures_;

    struct DescriptorPoolEntry
    {
        vk::raii::DescriptorPool pool{nullptr};
        std::vector<vk::raii::DescriptorSet> sets;
    };
    std::vector<DescriptorPoolEntry> descriptorPools_;

    // Flat lookup for descriptor set handles
    std::vector<vk::DescriptorSet> descriptorSetTable_;

    struct PipelineEntry
    {
        vk::Pipeline pipeline{};
        vk::PipelineLayout layout{};
    };
    std::vector<PipelineEntry> pipelines_;
};

} // namespace fire_engine
