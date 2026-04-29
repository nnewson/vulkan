#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include <fire_engine/graphics/gpu_handle.hpp>
#include <fire_engine/graphics/sampler_settings.hpp>
#include <fire_engine/graphics/texture.hpp>
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
    enum class FallbackTextureKind
    {
        BaseColour,
        Emissive,
        Normal,
        MetallicRoughness,
        Occlusion,
    };

    Resources(const Device& device, const Pipeline& pipeline);
    ~Resources() = default;

    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Resources(Resources&&) noexcept = default;
    Resources& operator=(Resources&&) noexcept = default;

    // --- Buffer creation ---

    [[nodiscard]] BufferHandle createVertexBuffer(std::span<const Vertex> vertices);
    [[nodiscard]] BufferHandle createIndexBuffer(std::span<const uint16_t> indices);
    [[nodiscard]] BufferHandle createIndexBuffer(std::span<const uint32_t> indices);

    // --- Texture creation ---

    [[nodiscard]] TextureHandle createTexture(const Image& image,
                                              const SamplerSettings& sampler = {},
                                              TextureEncoding encoding = TextureEncoding::Srgb);
    [[nodiscard]] TextureHandle createTexture(const uint8_t* pixels, int width, int height,
                                              const SamplerSettings& sampler = {},
                                              TextureEncoding encoding = TextureEncoding::Srgb);
    [[nodiscard]] TextureHandle createTexture(const float* pixels, int width, int height,
                                              const SamplerSettings& sampler = {});
    [[nodiscard]] TextureHandle createCubemapTexture(const float* pixels, uint32_t faceExtent,
                                                     const SamplerSettings& sampler = {});
    [[nodiscard]] TextureHandle createCubemapTexture(const float* pixels, uint32_t faceExtent,
                                                     uint32_t mipLevels,
                                                     const SamplerSettings& sampler = {});
    [[nodiscard]] TextureHandle createRenderTargetCubemap(uint32_t faceExtent, uint32_t mipLevels,
                                                          vk::Format format,
                                                          const SamplerSettings& sampler = {});
    [[nodiscard]] TextureHandle fallbackTexture(FallbackTextureKind kind);

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
        TextureHandle transmissionTexture{NullTexture};
    };

    struct ObjectDescriptorRequest
    {
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> uniformBufs{NullBuffer, NullBuffer};
        std::array<BufferHandle, MAX_FRAMES_IN_FLIGHT> lightBufs{NullBuffer, NullBuffer};
        TextureHandle shadowMap{NullTexture};
        TextureHandle irradianceMap{NullTexture};
        TextureHandle prefilteredMap{NullTexture};
        TextureHandle brdfLut{NullTexture};
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
    // layerCount > 1 produces a 2D array image for cascaded shadow mapping;
    // per-layer 2D views are created for framebuffer attachment.
    [[nodiscard]] TextureHandle createShadowMap(uint32_t extent, uint32_t layerCount = 1);

    // Per-layer 2D view on a shadow-map array (framebuffer attachment use).
    [[nodiscard]] vk::ImageView vulkanShadowMapLayerView(TextureHandle handle,
                                                         uint32_t layer) const noexcept;

    // Non-comparison sampler over the shadow image — used by PCSS to read raw
    // depth values during the blocker search. Same image as vulkanSampler().
    [[nodiscard]] vk::Sampler vulkanShadowSamplerLinear(TextureHandle handle) const noexcept;

    // MoltenVK workaround: depth-only render passes fail to store on Metal
    // TBDR. Creates a throwaway B8G8R8A8 colour attachment so the shadow pass
    // becomes a real (colour + depth) render pass. Contents are never read.
    [[nodiscard]] TextureHandle createShadowColourAttachment(uint32_t extent);

    // Allocates an R16G16B16A16_SFLOAT colour image sized to the given extent,
    // usable as both a colour attachment (forward pass target) and a sampled
    // texture (post-process input). Linear-filter sampler, ClampToEdge.
    [[nodiscard]] TextureHandle createOffscreenColourTarget(vk::Extent2D extent);

    // Multi-mip 2D HDR target used by the bloom downsample/upsample chain.
    // Per-mip 2D views are created for framebuffer attachment + shader input.
    [[nodiscard]] TextureHandle createBloomChain(uint32_t width, uint32_t height,
                                                 uint32_t mipLevels);

    [[nodiscard]] vk::ImageView vulkanBloomMipView(TextureHandle handle,
                                                   uint32_t mipLevel) const noexcept;

    // Releases an existing offscreen / shadow texture entry so it can be
    // rebuilt at a new extent (e.g. on swapchain resize). The handle is
    // invalidated; callers must replace it with the result of a subsequent
    // createOffscreenColourTarget / createShadowColourAttachment call.
    void releaseTexture(TextureHandle handle);

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

    [[nodiscard]] std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
    createUboImageSamplerDescriptors(vk::DescriptorSetLayout layout, const MappedBufferSet& ubo,
                                     vk::DeviceSize uboSize, TextureHandle texture);

    [[nodiscard]] std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
    createSkyboxDescriptors(vk::DescriptorSetLayout layout, const MappedBufferSet& skyboxUbo,
                            vk::DeviceSize skyboxUboSize, TextureHandle texture,
                            const MappedBufferSet& lightUbo, vk::DeviceSize lightUboSize);

    // Allocates MAX_FRAMES_IN_FLIGHT descriptor sets for a layout with a
    // single combined-image-sampler binding at slot 0, writing the texture's
    // view + sampler into each. Used by the post-process pass to sample the
    // forward HDR target.
    [[nodiscard]] std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
    createSingleImageSamplerDescriptors(vk::DescriptorSetLayout layout, TextureHandle texture);

    // Rewrites an existing descriptor set array (per-frame) so slot 0
    // references a new texture. Used on resize to rebind the post-process
    // input without reallocating sets.
    void updateSingleImageSamplerDescriptors(
        const std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>& sets, TextureHandle texture);

    // Creates a single descriptor set with one combined-image-sampler binding
    // bound to the supplied (view, sampler) pair. Used by bloom passes where
    // the input view changes per pass (different mip).
    [[nodiscard]] DescriptorSetHandle createImageViewDescriptor(vk::DescriptorSetLayout layout,
                                                                vk::ImageView view,
                                                                vk::Sampler sampler);

    // Allocates a 2-binding descriptor set array for the post-process pass:
    // binding 0 = HDR target, binding 1 = bloom mip 0 (additive bloom).
    [[nodiscard]] std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>
    createPostProcessDescriptors(vk::DescriptorSetLayout layout, TextureHandle hdrTarget,
                                 TextureHandle bloomChain);

    // Rewrites the post-process descriptor array so its two bindings target a
    // new HDR view + bloom mip 0. Used on resize.
    void
    updatePostProcessDescriptors(const std::array<DescriptorSetHandle, MAX_FRAMES_IN_FLIGHT>& sets,
                                 TextureHandle hdrTarget, TextureHandle bloomChain);

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

    // --- Shared shadow map (bound to every forward descriptor set) ---

    void shadowMap(TextureHandle handle) noexcept
    {
        shadowMap_ = handle;
    }

    [[nodiscard]] TextureHandle shadowMap() const noexcept
    {
        return shadowMap_;
    }

    void irradianceMap(TextureHandle handle) noexcept
    {
        irradianceMap_ = handle;
    }

    [[nodiscard]] TextureHandle irradianceMap() const noexcept
    {
        return irradianceMap_;
    }

    void prefilteredMap(TextureHandle handle) noexcept
    {
        prefilteredMap_ = handle;
    }

    [[nodiscard]] TextureHandle prefilteredMap() const noexcept
    {
        return prefilteredMap_;
    }

    void brdfLut(TextureHandle handle) noexcept
    {
        brdfLut_ = handle;
    }

    [[nodiscard]] TextureHandle brdfLut() const noexcept
    {
        return brdfLut_;
    }

    // --- Pipeline registry ---
    // Pipelines are owned elsewhere (by Pipeline class); Resources stores raw
    // handles so Renderer can resolve PipelineHandle values stamped on
    // DrawCommands to the Vulkan pipeline/layout pair to bind.

    [[nodiscard]] PipelineHandle registerPipeline(vk::Pipeline pipeline, vk::PipelineLayout layout);

    // --- Vulkan accessors (for Renderer command recording) ---

    [[nodiscard]] vk::Buffer vulkanBuffer(BufferHandle handle) const noexcept;
    [[nodiscard]] vk::Image vulkanImage(TextureHandle handle) const noexcept;
    [[nodiscard]] vk::ImageView vulkanImageView(TextureHandle handle) const noexcept;
    [[nodiscard]] vk::ImageView vulkanCubemapFaceView(TextureHandle handle, uint32_t face,
                                                      uint32_t mipLevel = 0) const noexcept;
    [[nodiscard]] vk::Sampler vulkanSampler(TextureHandle handle) const noexcept;
    [[nodiscard]] vk::Format textureFormat(TextureHandle handle) const noexcept;
    [[nodiscard]] uint32_t textureMipLevels(TextureHandle handle) const noexcept;
    [[nodiscard]] vk::DescriptorSet vulkanDescriptorSet(DescriptorSetHandle handle) const noexcept;
    [[nodiscard]] vk::Pipeline vulkanPipeline(PipelineHandle handle) const noexcept;
    [[nodiscard]] vk::PipelineLayout vulkanPipelineLayout(PipelineHandle handle) const noexcept;

private:
    struct DescriptorPoolEntry;

    BufferHandle storeBuffer(vk::raii::Buffer buf, vk::raii::DeviceMemory mem);
    DescriptorPoolEntry& createDescriptorPool(std::span<const vk::DescriptorPoolSize> poolSizes,
                                              uint32_t maxSets);
    [[nodiscard]] std::vector<vk::raii::DescriptorSet>
    allocateDescriptorSets(vk::DescriptorPool pool, vk::DescriptorSetLayout layout,
                           uint32_t count) const;
    [[nodiscard]] DescriptorSetHandle registerDescriptorSet(vk::DescriptorSet set);
    void retainDescriptorSets(DescriptorPoolEntry& poolEntry,
                              std::vector<vk::raii::DescriptorSet>& sets);
    [[nodiscard]] TextureHandle createFallbackTexture(FallbackTextureKind kind);

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
        // Shadow-map only: a second sampler with compareEnable=false, used by
        // the PCSS blocker search to read raw depth values from the same image.
        vk::raii::Sampler samplerLinear{nullptr};
        vk::Format format{vk::Format::eUndefined};
        std::vector<vk::raii::ImageView> faceViews;
        uint32_t mipLevels{1};
    };
    static void createCubemapFaceViews(const Device& device, TextureEntry& entry);
    std::vector<TextureEntry> textures_;
    std::array<TextureHandle, 5> fallbackTextures_{NullTexture, NullTexture, NullTexture,
                                                   NullTexture, NullTexture};

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
    TextureHandle shadowMap_{NullTexture};
    TextureHandle irradianceMap_{NullTexture};
    TextureHandle prefilteredMap_{NullTexture};
    TextureHandle brdfLut_{NullTexture};
    vk::DescriptorSetLayout shadowDescLayout_{};
};

} // namespace fire_engine
