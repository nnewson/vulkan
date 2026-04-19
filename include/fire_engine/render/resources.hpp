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
        TextureHandle emissiveTexture{NullTexture};
        TextureHandle normalTexture{NullTexture};
        TextureHandle metallicRoughnessTexture{NullTexture};
        TextureHandle occlusionTexture{NullTexture};
    };

    struct ObjectDescriptorRequest
    {
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> uniformBufs{NullBuffer, NullBuffer};
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> lightBufs{NullBuffer, NullBuffer};
        std::vector<GeometryDescriptorInfo> geometries;
    };

    struct ObjectDescriptorResult
    {
        // descSets[geometryIndex][frameIndex]
        std::vector<std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>> descSets;
    };

    [[nodiscard]] ObjectDescriptorResult
    createObjectDescriptors(const ObjectDescriptorRequest& req);

    // --- Shadow map + shadow descriptors ---

    // Allocates a 2D D32_SFLOAT image + view usable as a depth attachment and
    // as a sampled texture. Sampler uses comparison mode (CompareOp::eLess,
    // eClampToBorder, white border) so the forward shader can do hardware PCF
    // via sampler2DShadow. Returned handle slots into the existing texture
    // registry — retrieve view/sampler via the vulkan* accessors.
    [[nodiscard]] TextureHandle createShadowMap(uint32_t extent);

    struct ShadowGeometryDescriptorInfo
    {
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> shadowUboBufs{NullBuffer, NullBuffer};
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> skinBufs{NullBuffer, NullBuffer};
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> morphUboBufs{NullBuffer, NullBuffer};
        BufferHandle morphSsbo{NullBuffer};
        std::size_t morphSsboSize{0};
    };

    struct ShadowDescriptorRequest
    {
        std::vector<ShadowGeometryDescriptorInfo> geometries;
    };

    struct ShadowDescriptorResult
    {
        std::vector<std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>> descSets;
    };

    [[nodiscard]] ShadowDescriptorResult
    createShadowDescriptors(const ShadowDescriptorRequest& req);

    void shadowDescriptorSetLayout(vk::DescriptorSetLayout layout) noexcept
    {
        shadowDescLayout_ = layout;
    }
    [[nodiscard]] vk::DescriptorSetLayout shadowDescriptorSetLayout() const noexcept
    {
        return shadowDescLayout_;
    }

    // Allocates MAX_FRAMES_IN_FLIGHT descriptor sets for a layout that has a
    // single uniform-buffer binding at slot 0. Writes one UBO descriptor per
    // frame from the provided MappedBufferSet. Used by passes whose layout is
    // simpler than the forward 6-binding layout (e.g. the skybox).
    [[nodiscard]] std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
    createSingleUboDescriptors(vk::DescriptorSetLayout layout, const MappedBufferSet& ubo,
                               vk::DeviceSize uboSize);

    // --- Shared light UBO (bound to every forward descriptor set) ---

    void lightBuffers(const std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT>& bufs) noexcept
    {
        lightBuffers_ = bufs;
    }

    [[nodiscard]] const std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT>&
    lightBuffers() const noexcept
    {
        return lightBuffers_;
    }

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

    std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> lightBuffers_{NullBuffer, NullBuffer};
    vk::DescriptorSetLayout shadowDescLayout_{};
};

} // namespace fire_engine
